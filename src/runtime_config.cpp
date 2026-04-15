#include "runtime_config.h"
#include "config.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

char g_stop_city[32];
char g_stop_name[32];
char g_device_title[64];
int  g_refresh_min = DEFAULT_REFRESH_MIN;

static const int ALLOWED_REFRESH[] = {1, 3, 5, 10, 30, 60};
static const int ALLOWED_REFRESH_COUNT = 6;

int refreshMinSanitize(int v) {
    int best = ALLOWED_REFRESH[0];
    int bestDiff = abs(v - best);
    for (int i = 1; i < ALLOWED_REFRESH_COUNT; i++) {
        int d = abs(v - ALLOWED_REFRESH[i]);
        if (d < bestDiff) { best = ALLOWED_REFRESH[i]; bestDiff = d; }
    }
    return best;
}

static const char* CONFIG_PATH = "/config.json";

static void copyStr(char* dst, size_t cap, const char* src) {
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

void runtimeConfigInitDefaults() {
    copyStr(g_stop_city, sizeof(g_stop_city), DEFAULT_STOP_CITY);
    copyStr(g_stop_name, sizeof(g_stop_name), DEFAULT_STOP_NAME);
    copyStr(g_device_title, sizeof(g_device_title), DEFAULT_DEVICE_TITLE);
    g_refresh_min = DEFAULT_REFRESH_MIN;
}

bool runtimeConfigLoad() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        return false;
    }
    if (!LittleFS.exists(CONFIG_PATH)) {
        Serial.println("No stored config");
        return false;
    }
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("Config parse error: %s\n", err.c_str());
        return false;
    }

    const char* city  = doc["stop_city"]  | "";
    const char* name  = doc["stop_name"]  | "";
    const char* title = doc["device_title"] | "";
    int refresh       = doc["refresh_min"] | DEFAULT_REFRESH_MIN;
    if (*city)  copyStr(g_stop_city, sizeof(g_stop_city), city);
    if (*name)  copyStr(g_stop_name, sizeof(g_stop_name), name);
    if (*title) copyStr(g_device_title, sizeof(g_device_title), title);
    g_refresh_min = refreshMinSanitize(refresh);

    Serial.printf("Loaded config: %s / %s / %s / %d min\n",
        g_stop_city, g_stop_name, g_device_title, g_refresh_min);
    return true;
}

bool runtimeConfigSave() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        return false;
    }
    JsonDocument doc;
    doc["stop_city"]    = g_stop_city;
    doc["stop_name"]    = g_stop_name;
    doc["device_title"] = g_device_title;
    doc["refresh_min"]  = g_refresh_min;

    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f) {
        Serial.println("Config write open failed");
        return false;
    }
    serializeJson(doc, f);
    f.close();
    Serial.println("Config saved");
    return true;
}
