#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include "config.h"
#include "runtime_config.h"
#include "portal.h"
#include "time_sync.h"
#include "api_client.h"
#include "display_renderer.h"

static unsigned long lastFetchMs = 0;
static bool timeSynced = false;

// Double-reset detection: a second reset within DRD_WINDOW_MS of boot
// forces the config portal. RTC user memory survives the reset button but
// not a full power cycle, which is exactly what we want.
//
// The window runs synchronously inside setup() — write MAGIC, delay, clear —
// so that by the time esptool (or anything else) resets the chip later, the
// marker is already gone and we won't get a false positive on the next boot.
static const uint32_t DRD_MAGIC = 0xD00DBEEFUL;
static const unsigned long DRD_WINDOW_MS = 2000;

static bool drdCheckAndArm() {
    uint32_t v = 0;
    ESP.rtcUserMemoryRead(0, &v, sizeof(v));
    bool doubled = (v == DRD_MAGIC);

    v = DRD_MAGIC;
    ESP.rtcUserMemoryWrite(0, &v, sizeof(v));
    delay(DRD_WINDOW_MS);
    v = 0;
    ESP.rtcUserMemoryWrite(0, &v, sizeof(v));

    return doubled;
}

static void fetchAndRender() {
    Departure deps[MAX_DEPARTURES];
    int count = 0;

    bool ok = fetchDepartures(deps, count, MAX_DEPARTURES);

    if (ok) {
        for (int i = 0; i < count; i++) {
            Serial.printf("  %s -> %s in %d min (delay %d)\n",
                deps[i].lineNumber.c_str(),
                deps[i].destination.c_str(),
                deps[i].minutesUntil(),
                deps[i].delayMinutes);
        }
        if (!NO_DISPLAY) displayRender(deps, count);
    } else {
        Serial.println("Fetch failed");
        if (!NO_DISPLAY) displayError("Keine Daten");
    }

    lastFetchMs = millis();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== VRR eInk Display ===");

    runtimeConfigInitDefaults();
    runtimeConfigLoad();

    bool doubleReset = drdCheckAndArm();
    if (doubleReset) Serial.println("Double reset detected — forcing portal");

    if (!NO_DISPLAY) {
        displayInit();
        displayBoot(doubleReset ? "Setup (2x Reset)" : "Verbinde...");
    }

    if (!NO_DISPLAY && (doubleReset || WiFi.SSID().length() == 0)) {
        displayPortal(PORTAL_AP_NAME);
    }

    if (!portalRun(doubleReset)) {
        Serial.println("Portal timed out / failed");
        if (!NO_DISPLAY) displayError("WiFi Setup");
        delay(3000);
        ESP.restart();
    }

    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());

    timeSynced = syncTime(TZ_INFO, NTP_SERVER);

    portalStartWebPortal();
    if (timeSynced) fetchAndRender();
}

void loop() {
    portalProcess();

    bool intervalElapsed =
        millis() - lastFetchMs >= (unsigned long)g_refresh_min * 60UL * 1000UL;

    if (intervalElapsed) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi lost — restarting to re-run portal logic");
            ESP.restart();
        }
        if (!timeSynced) {
            timeSynced = syncTime(TZ_INFO, NTP_SERVER);
            if (!timeSynced) return;
        }
        fetchAndRender();
    }
}
