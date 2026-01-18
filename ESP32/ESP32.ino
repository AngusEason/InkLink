#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#include "EPD_7in5h.h"
#include "DEV_Config.h"

//#define HOME_SCREEN
#define MCLAREN_SCREEN

/* ================= USER CONFIG ================= */
#ifdef HOME_SCREEN
const char* WIFI_SSID      = "gigacube-645F1B";
const char* WIFI_PASSWORD = "6fMjmT5ggyr3523T";
const char* CRC_URL       = "https://eink-uploader-server-production.up.railway.app/static/home_crc32.bin";
const char* IMAGE_URL     = "https://eink-uploader-server-production.up.railway.app/static/home_image.bin";
#endif

#ifdef MCLAREN_SCREEN
const char* WIFI_SSID      = "gigacube-645F1B";
const char* WIFI_PASSWORD = "6fMjmT5ggyr3523T";
const char* CRC_URL       = "https://eink-uploader-server-production.up.railway.app/static/work_crc32.bin";
const char* IMAGE_URL     = "https://eink-uploader-server-production.up.railway.app/static/work_image.bin";
#endif

/* Poll interval */
#define CHECK_INTERVAL_MS (60UL * 1000UL)   // 1 minute

/* Display parameters */
#define IMAGE_WIDTH   800
#define IMAGE_HEIGHT  480
#define IMAGE_SIZE    (IMAGE_WIDTH * IMAGE_HEIGHT / 4)

#define IMAGE_PATH    "/image.bin"
#define TEMP_PATH     "/image_tmp.bin"

/* ================= Runtime CRC ================= */
static uint32_t lastImageCRC = 0;

/* ================= Fetch CRC ================= */
bool fetchServerCRC(uint32_t &outCRC) {
    HTTPClient http;
    http.begin(CRC_URL);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("CRC fetch failed: %d\n", code);
        http.end();
        return false;
    }

    if (http.getSize() != 4) {
        Serial.println("CRC file size invalid");
        http.end();
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    stream->readBytes((uint8_t*)&outCRC, 4);

    http.end();
    return true;
}

/* ================= Download Image ================= */
bool downloadImage() {
    Serial.println("Downloading image...");
    HTTPClient http;
    http.begin(IMAGE_URL);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", code);
        http.end();
        return false;
    }

    int len = http.getSize();
    if (len != IMAGE_SIZE) {
        Serial.printf("Size mismatch (%d != %d)\n", len, IMAGE_SIZE);
        http.end();
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    File f = LittleFS.open(TEMP_PATH, "w");
    if (!f) {
        Serial.println("Failed to open temp file");
        http.end();
        return false;
    }

    uint8_t buf[512];
    size_t total = 0;

    while (http.connected() && total < IMAGE_SIZE) {
        int avail = stream->available();
        if (avail) {
            int r = stream->readBytes(buf, min(avail, (int)sizeof(buf)));
            f.write(buf, r);
            total += r;
        }
        delay(1);
    }

    f.close();
    http.end();

    if (total != IMAGE_SIZE) {
        Serial.println("Download incomplete");
        LittleFS.remove(TEMP_PATH);
        return false;
    }

    if (LittleFS.exists(IMAGE_PATH)) LittleFS.remove(IMAGE_PATH);
    LittleFS.rename(TEMP_PATH, IMAGE_PATH);

    Serial.println("Image saved");
    return true;
}


/* ================= Display ================= */
const UWORD slice_height = 120;
const UWORD width_bytes = (EPD_7IN5H_WIDTH % 4 == 0) ? (EPD_7IN5H_WIDTH / 4) : (EPD_7IN5H_WIDTH / 4 + 1);
static uint8_t sliceBuffer[slice_height * width_bytes] __attribute__((aligned(4)));

void displayImage() {
    Serial.println("Opening image from LittleFS for display...");
    File f = LittleFS.open(IMAGE_PATH, "r");
    if (!f) {
        Serial.println("Failed to open image for display!");
        return;
    }
    Serial.println("LittleFS file opened.");


    UWORD ystart = 0;
    Serial.println("Starting display refresh in slices...");

    EPD_7IN5H_SendCommand(DATA_START_TRANSMISSION_COMMAND);
    while (ystart < EPD_7IN5H_HEIGHT) {
        UWORD rows = min(slice_height, (uint16_t)(EPD_7IN5H_HEIGHT - ystart));
        size_t bytesToRead = width_bytes * rows;
        size_t readBytes = f.read(sliceBuffer, bytesToRead);
        if (readBytes != bytesToRead) {
            Serial.printf("Error reading slice at y=%d\n", ystart);
            break;
        }

        for (int j = 0; j < rows; j++) {
            for (int i = 0; i < width_bytes; i++) {
                EPD_7IN5H_SendData(sliceBuffer[i + j * width_bytes]);
            }
        }

        Serial.printf("Loaded slice at y=%d\n", ystart);
        ystart += rows;
        yield();
    }

    f.close();

    EPD_7IN5H_TurnOnDisplay();
    Serial.println("Display refresh complete.");
}

/* ================= SETUP ================= */
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    Serial.println("Initializing e-paper display...");

    DEV_Module_Init();
    EPD_7IN5H_Init();
    EPD_7IN5H_Clear(EPD_7IN5H_WHITE);

    Serial.println("Initializing LittleFS...");
    if (!LittleFS.begin(false)) {   // try mount WITHOUT formatting
        Serial.println("LittleFS mount failed, formatting...");
        if (!LittleFS.begin(true)) {   // format + mount
            Serial.println("LittleFS format failed!");
            while (1) {
                delay(1000);
            }
        }
        Serial.println("LittleFS formatted successfully");
    }

    Serial.println("LittleFS mounted successfully");
}

/* ================= LOOP ================= */
unsigned long lastCheck = 0;

void loop() {
    unsigned long now = millis();
    if (now - lastCheck >= CHECK_INTERVAL_MS) {
        lastCheck = now;

        uint32_t serverCRC;
        Serial.println("Checking for new image on server...");
        if (!fetchServerCRC(serverCRC)) return;
        Serial.println("Current image CRC32: " + String(lastImageCRC, HEX) + "    Server image CRC32: " + String(serverCRC, HEX));
        if (serverCRC != lastImageCRC) {
            Serial.println("New image detected");
            if (downloadImage()) {
                displayImage();
                lastImageCRC = serverCRC;
            }
        } else {
            Serial.println("Image unchanged");
        }
    }
}
