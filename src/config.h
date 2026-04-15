#pragma once

// Non-sensitive, non-runtime config. Wiring, API endpoint, timings.
// Stop/title/WiFi are configured via the captive portal — see runtime_config.h.

// ----- Defaults used on first boot (and as portal placeholders) -----
#define DEFAULT_STOP_CITY     "Essen"
#define DEFAULT_STOP_NAME     "Hauptbahnhof"
#define DEFAULT_DEVICE_TITLE  "VRR eInk Display"

// ----- API -----
const char* const API_HOST = "openservice-test.vrr.de";
const char* const API_BASE = "/openservice/XML_DM_REQUEST"
    "?type_dm=stop"
    "&outputFormat=rapidJSON"
    "&useRealtime=1"
    "&mode=direct"
    "&limit=10";

// Number of data rows drawn below the column header (Linie/Richtung/Abfahrt).
const int MAX_DEPARTURES = 4;

// ----- Timing -----
// Default refresh interval in minutes. User-selectable via portal from
// {1, 3, 5, 10, 30, 60}. Persisted in runtime_config.
#define DEFAULT_REFRESH_MIN 5

// ----- NTP -----
const char* const NTP_SERVER = "pool.ntp.org";
const char* const TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// ----- Portal -----
// AP shown when no WiFi is stored or stored creds fail to connect.
#define PORTAL_AP_NAME     "VRR-Display-Setup"
#define PORTAL_TIMEOUT_SEC 180

// ----- Debug -----
#define NO_DISPLAY false

// ----- Display pins (Wemos D1 Mini) -----
// SCK=D5/GPIO14, MOSI=D7/GPIO13 are fixed hardware SPI.
// Avoid D8/GPIO15 (must be LOW at boot) and D4/GPIO2 (must be HIGH at boot).
#define EPD_CS    5   // D1
#define EPD_DC    0   // D3 — shared with PORTAL_TRIGGER_PIN (FLASH button)
#define EPD_RST   16  // D0
#define EPD_BUSY  4   // D2
