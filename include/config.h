#pragma once

// Uplink (home/router WiFi). Leave empty — configure via USB serial: uplink <ssid> <pass>
#ifndef UPLINK_SSID
#define UPLINK_SSID ""
#endif
#ifndef UPLINK_PASS
#define UPLINK_PASS ""
#endif

// Guest hotspot AP. Leave empty to use AP_SSID/AP_PASS from EEPROM after serial setup.
#ifndef AP_SSID
#define AP_SSID "~°7____"
#endif
#ifndef AP_PASS
#define AP_PASS ""
#endif

// Per-deployment pepper for AES-encrypted credentials in flash (change before field deploy).
// Override in platformio.ini: -DCREDENTIAL_PEPPER=\"your-secret\"

// OLED carousel: status → stats → matrix owl (ms)
#ifndef OLED_SCREEN_INTERVAL_MS
#define OLED_SCREEN_INTERVAL_MS 12444
#endif

// Default web UI title (overridable via serial: title <name>)
#ifndef PAGE_TITLE_DEFAULT
#define PAGE_TITLE_DEFAULT "Tribe Repeater Node"
#endif

// Privacy / security notes (not TOR):
// - Operator uplink SSID/password are AES-encrypted in EEPROM/NVS (chip ID + CREDENTIAL_PEPPER).
// - Web /config and /settings are disabled; use serial admin at 115200 (help).
// - STA MAC is randomized on each uplink connect to reduce donor-router fingerprinting.
// - AP client isolation: ESP32 limited by IDF; ESP8266 unavailable — map hides client MACs when privacy on.
// - Guests should use VPN/Tor on their own devices for internet anonymity.

// --- Your three boards (use the matching env; never flash ESP32 firmware to ESP8266) ---
//
//  | # | Hardware                              | PlatformIO env        | What it runs
//  |---|---------------------------------------|-----------------------|---------------------------
//  | 1 | ESP8266 + 0.96" OLED (D3/D4 / GPIO2,0) | esp8266_az (autodetect alt bus)
//  | 2 | Ideaspark ESP32-WROOM + onboard OLED       | esp32dev  (GPIO 21/22)
//  |   | ZY-ESP32 (ESP32-WROOM, WiFi + BLE, CP2102) | zy_esp32  (NOT S3 — use ttyUSB1)
//  | 3 | ESP8266MOD D1 + 0.96" OLED on D14/D15 headers | esp8266_az (GPIO 4/5, not 14/15!)
//
// Upload:
//   pio run -t upload -e esp8266_az --upload-port /dev/ttyUSB0
//   pio run -t upload -e zy_esp32    --upload-port /dev/ttyUSB1
//   pio run -t upload -e esp32dev    --upload-port /dev/ttyUSBx
//
// Raspberry Pi 2 hub (Raspbian Lite 32-bit): OLED + AP + HTTP + Tor proxy for guests
//   See pi/README.md — sudo bash pi/install.sh
//   ESP/8266/D1 nodes join AP "TribeHub-Pi" or Pi joins ESP "TribeNode-*" uplink
