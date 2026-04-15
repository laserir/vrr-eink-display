# vrr-eink-display

A tiny battery-friendly departure monitor for VRR (Verkehrsverbund Rhein-Ruhr) stops in Germany's Ruhr area. Runs on a Wemos D1 Mini (ESP8266) and a Waveshare 2.9" 3-color ePaper, and renders the next departures from any VRR stop.

Configuration is done through a captive portal — no recompiling to change stops, no secrets baked into firmware.

## Features

- Compact layout for the Waveshare 2.9" B (296×128, black/white/red)
- Live VRR openservice API with real-time delays
- Delayed departures highlighted in red
- Captive portal (WiFiManager) for WiFi + setup (stop city, stop name, device title, refresh interval)

## Hardware

- **Wemos D1 Mini** (ESP8266) — [wemos.cc/en/latest/d1/d1_mini.html](https://www.wemos.cc/en/latest/d1/d1_mini.html)
- **Waveshare 2.9" e-Paper Module (B)** — 3-color (black/white/red), 296×128, SSD1680, GDEM029C90 panel — [waveshare.com/2.9inch-e-paper-module-b.htm](https://www.waveshare.com/2.9inch-e-paper-module-b.htm) (ships with an 8-wire dupont cable — all the wiring you need)
- **Optional 3D-printed case** — [Live E-Paper Transit Departure Board on MakerWorld](https://makerworld.com/de/models/2583544-live-e-paper-transit-departure-board#profileId-2849733) fits the Wemos + 2.9" Waveshare B perfectly

### Wiring

| Wemos D1 Mini | GPIO    | ePaper      | Symbol    |
|---------------|---------|-------------|-----------|
| 3V3           | —       | VCC         | —         |
| GND           | —       | GND         | —         |
| D7            | GPIO13  | DIN (MOSI)  | HW SPI    |
| D5            | GPIO14  | CLK (SCK)   | HW SPI    |
| D1            | GPIO5   | CS          | `EPD_CS`  |
| D3            | GPIO0   | DC          | `EPD_DC`  |
| D0            | GPIO16  | RST         | `EPD_RST` |
| D2            | GPIO4   | BUSY        | `EPD_BUSY`|

See `wiring.html` for a visual diagram. Measure VCC directly at the ePaper connector with everything plugged in; it should be ≥ 3.0 V. If it's lower, your dupont wires are lossy — solder direct.

## Flashing the firmware

Grab `firmware.bin` from the latest [GitHub release](../../releases). You have two options to write it to the ESP:

### Option A — Spacehuhn Web Flasher (no install)

1. Plug the Wemos into your computer via USB.
2. Open [**esp.huhn.me**](https://esp.huhn.me/) in Chrome or Edge (WebSerial is required).
3. Click **Connect** and pick the Wemos serial port.
4. Drop `firmware.bin` into the *Flash* slot at offset `0x0000`.
5. Click **Program**. Wait for "Done".

### Option B — `esptool.py` (command line)

```bash
pip install esptool
esptool.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 \
    write_flash 0x0 firmware.bin
```

(Replace `/dev/ttyUSB0` with `COM3` or similar on Windows.)

After flashing, the Wemos reboots into the setup portal on first run.

## First-time setup

1. The ePaper shows a setup screen: connect to the **`VRR-Display-Setup`** WiFi from your phone.
2. A captive portal opens (or browse to `192.168.4.1`).
3. In the menu, open **Setup**, fill in stop city / stop name / device title / refresh and hit **Save**. Then open **Configure WiFi**, pick your network, enter the password and hit **Save**. The device connects and starts showing departures.

## Changing settings later

There are two ways to reach the configuration:

**1. Over the network (easy path).** Browse to **`http://<device-ip>/`** (the IP is printed on the serial console at boot and shows up in your router's client list as `ESP-…`). It's the same portal page, just reached over your LAN.

> The config page has no authentication — anyone on your LAN can change the stop. This is a home gadget; keep it on a trusted network.

**2. Double-reset → captive portal (when the network path is unreachable — e.g. new WiFi).** Press the **RST** button on the Wemos, and as soon as the display starts to update, press **RST** again within ~2 seconds. The device boots into the `VRR-Display-Setup` AP - just like First-time setup. Join it from your phone, and in the portal menu open **Setup** to edit stop / title / refresh without re-entering WiFi, or **Configure WiFi** to switch networks. A single reset just reboots the device normally — only two quick resets trigger the portal.

## Finding your stop name

VRR's openservice API matches `name_dm` against what's in their own database. Usually the German stop name works directly (`Hauptbahnhof`, `Rüttenscheider Stern`, etc.). If nothing shows up, try the name used on [vrr.de](https://www.vrr.de/) route planning.

## Libraries used

| Library | Version | License |
|---|---|---|
| [GxEPD2](https://github.com/ZinggJM/GxEPD2) | 1.6.8 | GPL-3.0 |
| [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) | (transitive via GxEPD2) | BSD |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | 7.4.3 | MIT |
| [WiFiManager](https://github.com/tzapu/WiFiManager) (tzapu) | 2.0.17 | MIT |
| [ESP8266 Arduino core](https://github.com/esp8266/Arduino) | platform default | LGPL-2.1 |

## License

MIT — see `LICENSE`. Note that when linked against GxEPD2 (GPL-3.0), the combined binary is effectively GPL-3.0.
