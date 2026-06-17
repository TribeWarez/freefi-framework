# FreeFi Framework

**Mesh WiFi NAT repeater nodes** — ESP8266 / ESP32 firmware + Raspberry Pi hub with OLED, web UI, IRC chat, and Tor guest proxy.

- [freefi.tribewarez.com](https://freefi.tribewarez.com)
- [docs.tribewarez.com](https://docs.tribewarez.com)
- [irc.tribewarez.com](https://irc.tribewarez.com)

## Quick start

```bash
pio run -e esp32dev -t upload
```

Configure uplink via serial: `uplink MyWiFi MyPassword`

## Boards

| Board | Env | OLED |
|---|---|---|
| ESP32-WROOM (Ideaspark) | `esp32dev` | GPIO 21/22 |
| ZY-ESP32 (CP2102) | `zy_esp32` | autodetect |
| Heltec WiFi Kit 32 | `heltec_wifi_kit_32` | built-in |
| ESP8266 + 0.96" OLED | `esp8266_az` | autodetect |

## Pi hub

See [pi/README.md](pi/README.md) for the Raspberry Pi hotspot + Tor guest proxy.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).
