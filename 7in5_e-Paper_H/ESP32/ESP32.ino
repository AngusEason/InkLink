#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#include "EPD_7in5h.h"
#include "DEV_Config.h"

/* ================= USER CONFIG ================= */
const char* WIFI_SSID     = "Angus_S24_FE";
const char* WIFI_PASSWORD = "youareachamp69";

/* Image URL (raw GitHub) */
const char* IMAGE_URL = "https://github.com/AngusEason/epaper-frame-server/raw/refs/heads/main/image.bin";

/* Poll interval */
#define CHECK_INTERVAL_MS (60UL * 1000UL)   // 1 minute

/* Display parameters */
#define IMAGE_WIDTH   800
#define IMAGE_HEIGHT  480
#define IMAGE_SIZE    (IMAGE_WIDTH * IMAGE_HEIGHT / 4)

#define IMAGE_PATH    "/image.bin"
#define TEMP_PATH     "/image_tmp.bin"

/* ================= CRC32 ================= */
uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    while (length--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        }
    }
    return ~crc;
}

/* ================= Download & Save ================= */
bool downloadImage() {
    Serial.println("Starting HTTP GET for new image...");
    HTTPClient http;
    http.begin(IMAGE_URL);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", code);
        http.end();
        return false;
    }

    int len = http.getSize();
    Serial.printf("HTTP OK. Content length: %d bytes\n", len);
    if (len != IMAGE_SIZE) {
        Serial.printf("Warning: size mismatch! Expected %d, got %d\n", IMAGE_SIZE, len);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();

    // Open temporary file for writing
    File f = LittleFS.open(TEMP_PATH, "w");
    if (!f) {
        Serial.println("Failed to open temp file for writing!");
        http.end();
        return false;
    }

    size_t total = 0;
    uint8_t buf[512]; // small buffer for streaming

    Serial.println("Downloading image in chunks...");
    while (http.connected() && total < IMAGE_SIZE) {
        int avail = stream->available();
        if (avail) {
            int toRead = min(avail, (int)sizeof(buf));
            int r = stream->readBytes(buf, toRead);
            f.write(buf, r);
            total += r;

            Serial.printf("Downloaded %d / %d bytes\n", total, IMAGE_SIZE);
        }
        delay(1);
    }

    f.close();
    http.end();

    if (total != IMAGE_SIZE) {
        Serial.println("Download incomplete!");
        LittleFS.remove(TEMP_PATH);
        return false;
    }

    // Compute CRC of new file
    f = LittleFS.open(TEMP_PATH, "r");
    uint8_t tempBuf[512];
    uint32_t crc = 0xFFFFFFFF;
    size_t remaining = IMAGE_SIZE;
    while (remaining > 0) {
        size_t chunk = min(remaining, (size_t)sizeof(tempBuf));
        f.read(tempBuf, chunk);
        crc ^= crc32(tempBuf, chunk);
        remaining -= chunk;
    }
    f.close();
    crc = ~crc;

    // Compare CRC with old file if exists
    uint32_t oldCRC = 0;
    if (LittleFS.exists(IMAGE_PATH)) {
        f = LittleFS.open(IMAGE_PATH, "r");
        remaining = IMAGE_SIZE;
        crc32(tempBuf, sizeof(tempBuf)); // just to ensure function compiled
        uint32_t tmpCRC = 0xFFFFFFFF;
        while (remaining > 0) {
            size_t chunk = min(remaining, (size_t)sizeof(tempBuf));
            f.read(tempBuf, chunk);
            tmpCRC ^= crc32(tempBuf, chunk);
            remaining -= chunk;
        }
        f.close();
        oldCRC = ~tmpCRC;
    }

    if (crc == oldCRC) {
        Serial.println("Image unchanged (CRC match). Not updating.");
        LittleFS.remove(TEMP_PATH);
        return false;
    }

    // Replace old file
    if (LittleFS.exists(IMAGE_PATH)) LittleFS.remove(IMAGE_PATH);
    LittleFS.rename(TEMP_PATH, IMAGE_PATH);
    Serial.println("New image saved to LittleFS.");
    return true;
}

/* ================= Display ================= */
void displayImage() {
    Serial.println("Opening image from LittleFS for display...");
    File f = LittleFS.open(IMAGE_PATH, "r");
    if (!f) {
        Serial.println("Failed to open image for display!");
        return;
    }

    const UWORD sliceHeight = 40; // 40 rows at a time (~4.8 KB buffer)
    const UWORD WidthBytes = (EPD_7IN5H_WIDTH % 4 == 0) ? (EPD_7IN5H_WIDTH / 4) : (EPD_7IN5H_WIDTH / 4 + 1);
    uint8_t buf[WidthBytes * sliceHeight];

    UWORD ystart = 0;
    while (ystart < EPD_7IN5H_HEIGHT) {
        UWORD rows = min(sliceHeight, EPD_7IN5H_HEIGHT - ystart);
        size_t bytesToRead = WidthBytes * rows;
        size_t readBytes = f.read(buf, bytesToRead);
        if (readBytes != bytesToRead) {
            Serial.printf("Error reading slice at y=%d\n", ystart);
            break;
        }

        EPD_7IN5H_DisplayPart(buf, 0, ystart, EPD_7IN5H_WIDTH, rows);
        ystart += rows;
        Serial.printf("Displayed slice at y=%d\n", ystart);
    }

    f.close();
    Serial.println("Display refresh complete.");
}

/* ================= SETUP ================= */
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts++;
        if (attempts > 60) {
            Serial.println("\nFailed to connect to WiFi!");
            while (1);
        }
    }
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());

    Serial.println("Initializing e-paper display...");
    DEV_Module_Init();
    EPD_7IN5H_Init();
    EPD_7IN5H_Clear(EPD_7IN5H_WHITE);
    delay(2000);

    Serial.println("Initializing LittleFS...");
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
        while (1);
    }
}

/* ================= LOOP ================= */
unsigned long lastCheck = 0;

void loop() {
    unsigned long now = millis();
    if (now - lastCheck >= CHECK_INTERVAL_MS) {
        lastCheck = now;

        Serial.println("Checking for new image on server...");
        if (downloadImage()) {
            Serial.println("Displaying new image...");
            displayImage();
        } else {
            Serial.println("No new image to display.");
        }
    }

    delay(50);
}
