#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "EPD_7in5h.h"
#include "DEV_Config.h"

#define HOME_SCREEN
//#define MCLAREN_SCREEN

/* ================= USER CONFIG ================= */
#ifdef HOME_SCREEN
const char* WIFI_SSID      = "gigacube-64F1B";
const char* WIFI_PASSWORD = "6fMjmT5ggyr3523T";
const char* META_URL      = "https://eink-uploader-home-server-production.up.railway.app/meta/home";
const char* IMAGE_URL     = "https://eink-uploader-home-server-production.up.railway.app/static/eink_home.bin";
#endif

#ifdef MCLAREN_SCREEN
const char* WIFI_SSID      = "McLaren Guest";
const char* WIFI_PASSWORD = "";
const char* META_URL      = "https://eink-uploader-home-server-production.up.railway.app/meta/work";
const char* IMAGE_URL     = "https://eink-uploader-home-server-production.up.railway.app/static/eink_work.bin";
#endif

/* Poll interval */
#define CHECK_INTERVAL_MS (300UL * 1000UL)   // 5 minutes

/* Display parameters */
#define IMAGE_WIDTH   800
#define IMAGE_HEIGHT  480
#define IMAGE_SIZE    (IMAGE_WIDTH * IMAGE_HEIGHT / 4)

#define IMAGE_PATH    "/image.bin"
#define TEMP_PATH     "/image_tmp.bin"
#define CRC_PATH      "/image.crc"

/* ================= CRC STORAGE ================= */
uint32_t loadStoredCRC() {
    if (!LittleFS.exists(CRC_PATH)) return 0;

    File f = LittleFS.open(CRC_PATH, "r");
    if (!f) return 0;

    uint32_t crc = f.parseInt();
    f.close();
    return crc;
}

void saveStoredCRC(uint32_t crc) {
    File f = LittleFS.open(CRC_PATH, "w");
    if (!f) return;
    f.printf("%lu", crc);
    f.close();
}

/* ================= METADATA CHECK ================= */
bool checkForNewImage(uint32_t &newCRC) {
    HTTPClient http;
    http.begin(META_URL);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("Metadata HTTP error: %d\n", code);
        http.end();
        return false;
    }

    DynamicJsonDocument doc(256);
    deserializeJson(doc, http.getString());
    http.end();

    newCRC = doc["crc32"];
    uint32_t size = doc["size"];

    if (size != IMAGE_SIZE) {
        Serial.println("Image size mismatch from server!");
        return false;
    }

    uint32_t oldCRC = loadStoredCRC();

    Serial.printf("Server CRC: %lu | Stored CRC: %lu\n", newCRC, oldCRC);

    return newCRC != oldCRC;
}

/* ================= Download & Save ================= */
bool downloadImage(uint32_t newCRC) {
    Serial.println("Downloading new image...");
    HTTPClient http;
    http.begin(IMAGE_URL);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", code);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    File f = LittleFS.open(TEMP_PATH, "w");
    if (!f) {
        Serial.println("Failed to open temp file!");
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
        Serial.println("Download incomplete!");
        LittleFS.remove(TEMP_PATH);
        return false;
    }

    if (LittleFS.exists(IMAGE_PATH)) LittleFS.remove(IMAGE_PATH);
    LittleFS.rename(TEMP_PATH, IMAGE_PATH);
    saveStoredCRC(newCRC);

    Serial.println("New image saved.");
    return true;
}

/* ================= Display ================= */
const UWORD slice_height = 120;
const UWORD width_bytes = (EPD_7IN5H_WIDTH % 4 == 0) ? (EPD_7IN5H_WIDTH / 4) : (EPD_7IN5H_WIDTH / 4 + 1);
static uint8_t sliceBuffer[slice_height * width_bytes] __attribute__((aligned(4)));

void displayImage() {
    File f = LittleFS.open(IMAGE_PATH, "r");
    if (!f) return;

    UWORD ystart = 0;
    EPD_7IN5H_SendCommand(DATA_START_TRANSMISSION_COMMAND);

    while (ystart < EPD_7IN5H_HEIGHT) {
        UWORD rows = min(slice_height, (uint16_t)(EPD_7IN5H_HEIGHT - ystart));
        f.read(sliceBuffer, width_bytes * rows);

        for (int j = 0; j < rows; j++)
            for (int i = 0; i < width_bytes; i++)
                EPD_7IN5H_SendData(sliceBuffer[i + j * width_bytes]);

        ystart += rows;
        yield();
    }

    f.close();
    EPD_7IN5H_TurnOnDisplay();
}

/* ================= SETUP ================= */
void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    DEV_Module_Init();
    EPD_7IN5H_Init();
    EPD_7IN5H_Clear(EPD_7IN5H_WHITE);

    if (!LittleFS.begin(false)) {
        LittleFS.begin(true);
    }
}

/* ================= LOOP ================= */
unsigned long lastCheck = 0;

void loop() {
    if (millis() - lastCheck >= CHECK_INTERVAL_MS) {
        lastCheck = millis();

        uint32_t newCRC;
        if (checkForNewImage(newCRC)) {
            if (downloadImage(newCRC)) {
                displayImage();
            }
        } else {
            Serial.println("No update needed.");
        }
    }
}
