#include "display_renderer.h"
#include "config.h"
#include "runtime_config.h"

#include <SPI.h>
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_3C.h>

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// FreeSans9pt7b only covers ASCII 0x20-0x7E, so umlauts (UTF-8 multi-byte)
// render as blank glyphs. Transliterate ä→ae, ö→oe, ü→ue, ß→ss (and caps)
// before printing any user/API-supplied text.
static String deUmlaut(const char* s) {
    String out;
    if (!s) return out;
    out.reserve(strlen(s));
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        if (p[0] == 0xC3 && p[1]) {
            unsigned char c = p[1];
            p++;
            switch (c) {
                case 0xA4: out += "ae"; continue;
                case 0xB6: out += "oe"; continue;
                case 0xBC: out += "ue"; continue;
                case 0x84: out += "Ae"; continue;
                case 0x96: out += "Oe"; continue;
                case 0x9C: out += "Ue"; continue;
                case 0x9F: out += "ss"; continue;
                default: continue;
            }
        }
        out += (char)*p;
    }
    return out;
}
static String deUmlaut(const String& s) { return deUmlaut(s.c_str()); }

// Waveshare 2.9" B (296x128, SSD1680, black/white/red)
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT>
    epd(GxEPD2_290_C90c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

static const int W = 296;
static const int H = 128;
static const int HEADER_H = 22;
static const int FOOTER_H = 12;
static const int ROW_H = 18;  // 5 rows fit in 90px
static const int LIST_TOP = HEADER_H + 2;

// Column geometry (pixels)
static const int COL_LINE_X = 4;
static const int COL_LINE_W = 42;
static const int COL_TIME_W = 44;
static const int COL_DEST_X = COL_LINE_X + COL_LINE_W;
static const int COL_TIME_X = W - COL_TIME_W - 2;
static const int COL_DEST_W = COL_TIME_X - COL_DEST_X - 4;

// Draws a stylised German "Haltestelle" sign (H in a ring) on the red
// header bar. cx/cy = center, r = outer ring radius.
static void drawHaltestelleSign(int cx, int cy, int r) {
    // Outer white filled disk
    epd.fillCircle(cx, cy, r, GxEPD_WHITE);
    // Inner red disk to leave a white ring
    epd.fillCircle(cx, cy, r - 2, GxEPD_RED);
    // H: two verticals + crossbar, drawn in white on the red center
    int hx = 3;      // half-width of the H
    int hy = r - 4;  // half-height of the H
    epd.drawLine(cx - hx, cy - hy, cx - hx, cy + hy, GxEPD_WHITE);
    epd.drawLine(cx - hx + 1, cy - hy, cx - hx + 1, cy + hy, GxEPD_WHITE);
    epd.drawLine(cx + hx, cy - hy, cx + hx, cy + hy, GxEPD_WHITE);
    epd.drawLine(cx + hx - 1, cy - hy, cx + hx - 1, cy + hy, GxEPD_WHITE);
    epd.drawLine(cx - hx, cy, cx + hx, cy, GxEPD_WHITE);
}

static void drawHeader() {
    epd.fillRect(0, 0, W, HEADER_H, GxEPD_RED);

    drawHaltestelleSign(11, HEADER_H / 2, 9);

    epd.setFont(&FreeSansBold9pt7b);
    epd.setTextColor(GxEPD_WHITE);
    epd.setCursor(24, HEADER_H - 6);
    epd.print(deUmlaut(g_stop_name));

    // Only render the header clock after NTP has given us a sane time.
    // Before that, `time()` returns the epoch and the corner would show 01:00.
    time_t now = time(nullptr);
    if (now < 100000) return;

    struct tm ti;
    localtime_r(&now, &ti);
    char timeBuf[6];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &ti);

    int16_t x1, y1;
    uint16_t tw, th;
    epd.getTextBounds(timeBuf, 0, 0, &x1, &y1, &tw, &th);
    epd.setCursor(W - tw - 4, HEADER_H - 6);
    epd.print(timeBuf);
}

// Truncates `s` with an ellipsis so its rendered width fits `maxW`.
// Uses the font currently set on `epd`.
static String fitText(const String& s, int maxW) {
    int16_t x1, y1;
    uint16_t tw, th;
    epd.getTextBounds(s.c_str(), 0, 0, &x1, &y1, &tw, &th);
    if ((int)tw <= maxW) return s;

    String base = s;
    while (base.length() > 1) {
        base.remove(base.length() - 1);
        String test = base + "..";
        epd.getTextBounds(test.c_str(), 0, 0, &x1, &y1, &tw, &th);
        if ((int)tw <= maxW) return test;
    }
    return String("");
}

static void drawRow(int y, const Departure& d) {
    // Line number (bold, black), truncate if wider than column
    epd.setFont(&FreeSansBold9pt7b);
    epd.setTextColor(GxEPD_BLACK);
    epd.setCursor(COL_LINE_X, y + 13);
    epd.print(fitText(deUmlaut(d.lineNumber), COL_LINE_W));

    // Destination (regular)
    epd.setFont(&FreeSans9pt7b);
    epd.setTextColor(GxEPD_BLACK);
    String dest = fitText(deUmlaut(d.destination), COL_DEST_W);
    epd.setCursor(COL_DEST_X, y + 13);
    epd.print(dest);

    // Departure clock time. Use estimated if present, else planned.
    time_t t = d.estimatedTime ? d.estimatedTime : d.plannedTime;
    struct tm ti;
    localtime_r(&t, &ti);
    char timeBuf[6];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &ti);

    epd.setFont(&FreeSansBold9pt7b);
    epd.setTextColor(d.delayMinutes > 2 ? GxEPD_RED : GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t tw, th;
    epd.getTextBounds(timeBuf, 0, 0, &x1, &y1, &tw, &th);
    epd.setCursor(W - tw - 4, y + 13);
    epd.print(timeBuf);
}

static void drawFooter() {
    int y = H - FOOTER_H;
    struct tm ti;
    time_t now = time(nullptr);
    localtime_r(&now, &ti);
    char buf[24];
    strftime(buf, sizeof(buf), "Akt. %H:%M", &ti);

    epd.setFont(nullptr);
    epd.setTextColor(GxEPD_BLACK);
    epd.setTextSize(1);

    epd.setCursor(2, y + 2);
    epd.print(deUmlaut(g_device_title));

    int16_t x1, y1;
    uint16_t tw, th;
    epd.getTextBounds(buf, 0, 0, &x1, &y1, &tw, &th);
    epd.setCursor(W - tw - 2, y + 2);
    epd.print(buf);
}

void displayInit() {
    epd.init(115200, true, 50, false);
    epd.setRotation(1);
}

void displayBoot(const char* statusLine) {
    epd.setFullWindow();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);

        epd.setFont(&FreeSansBold9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;
        String title = deUmlaut(g_device_title);
        epd.getTextBounds(title.c_str(), 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, H / 2 - 6);
        epd.print(title);

        epd.setFont(&FreeSans9pt7b);
        epd.setTextColor(GxEPD_RED);
        String status = deUmlaut(statusLine);
        epd.getTextBounds(status.c_str(), 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, H / 2 + 16);
        epd.print(status);
    } while (epd.nextPage());
}

void displayRender(Departure* deps, int count) {
    epd.setFullWindow();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        drawHeader();

        if (count == 0) {
            epd.setFont(&FreeSans9pt7b);
            epd.setTextColor(GxEPD_BLACK);
            const char* msg = "Keine Abfahrten";
            int16_t x1, y1;
            uint16_t tw, th;
            epd.getTextBounds(msg, 0, 0, &x1, &y1, &tw, &th);
            epd.setCursor((W - tw) / 2, H / 2 + 4);
            epd.print(msg);
        } else {
            // Column header row (small built-in font)
            epd.setFont(nullptr);
            epd.setTextSize(1);
            epd.setTextColor(GxEPD_BLACK);
            int hy = LIST_TOP + 4;
            epd.setCursor(COL_LINE_X, hy);
            epd.print("Linie");
            epd.setCursor(COL_DEST_X, hy);
            epd.print("Richtung");
            const char* h = "Abfahrt";
            int16_t hx1, hy1;
            uint16_t hw, hh;
            epd.getTextBounds(h, 0, 0, &hx1, &hy1, &hw, &hh);
            epd.setCursor(W - hw - 4, hy);
            epd.print(h);
            // Separator line below the header
            epd.drawLine(2, LIST_TOP + ROW_H - 3, W - 2, LIST_TOP + ROW_H - 3, GxEPD_BLACK);

            for (int i = 0; i < count && i < MAX_DEPARTURES; i++) {
                drawRow(LIST_TOP + (i + 1) * ROW_H, deps[i]);
            }
        }

        drawFooter();
    } while (epd.nextPage());
}

void displayError(const char* message) {
    epd.setFullWindow();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        drawHeader();

        epd.setFont(&FreeSans9pt7b);
        epd.setTextColor(GxEPD_RED);
        int16_t x1, y1;
        uint16_t tw, th;
        String msg = deUmlaut(message);
        epd.getTextBounds(msg.c_str(), 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, H / 2 + 10);
        epd.print(msg);
    } while (epd.nextPage());
}

void displayPortal(const char* apName) {
    epd.setFullWindow();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        epd.fillRect(0, 0, W, HEADER_H, GxEPD_RED);
        epd.setFont(&FreeSansBold9pt7b);
        epd.setTextColor(GxEPD_WHITE);
        epd.setCursor(4, HEADER_H - 6);
        epd.print("Setup");

        epd.setFont(&FreeSans9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        int16_t x1, y1;
        uint16_t tw, th;

        const char* l1 = "Verbinde dich mit WLAN:";
        epd.getTextBounds(l1, 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, HEADER_H + 22);
        epd.print(l1);

        epd.setFont(&FreeSansBold9pt7b);
        epd.setTextColor(GxEPD_RED);
        epd.getTextBounds(apName, 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, HEADER_H + 44);
        epd.print(apName);

        epd.setFont(&FreeSans9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        const char* l3 = "dann 192.168.4.1 im Browser";
        epd.getTextBounds(l3, 0, 0, &x1, &y1, &tw, &th);
        epd.setCursor((W - tw) / 2, HEADER_H + 66);
        epd.print(l3);
    } while (epd.nextPage());
}
