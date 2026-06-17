# Tribe Hub — Raspberry Pi 2 (32-bit Raspbian Lite)

Full **hotspot + HTTP UI + OLED** like ESP nodes, plus **Tor SOCKS/transparent proxy** for guests. Any **ESP8266 / ESP32 / D1** Tribe node can connect to this hub directly.

## Hardware

| Part | Notes |
|------|--------|
| Pi 2 (armhf) | Raspbian Lite 32-bit (Buster/Bullseye) |
| 0.96″ SSD1306 OLED | I2C: SDA GPIO2, SCL GPIO3 (same as ESP32 21/22 layout on Pi header) |
| WiFi | **Recommended:** USB WiFi dongle = `wlan1` for scan/uplink while `wlan0` = AP |
| Ethernet | Optional uplink via `eth0` (faster than ESP uplink) |

Enable I2C: `sudo raspi-config` → Interface Options → I2C → Enable.

## Install (on the Raspberry Pi)

Copy the `pi/` folder to the Pi, then:

```bash
cd pi
sudo bash install.sh
sudo bash post-install.sh
```

`post-install.sh` applies config, starts hostapd/dnsmasq/tribe-node, and enables Tor guest routing if `tor_guest_proxy` is true in config.

Manual steps if needed:

```bash
sudo /opt/tribe-node/pi/scripts/setup-ap-network.sh
sudo systemctl unmask hostapd dnsmasq
sudo systemctl start hostapd dnsmasq tor tribe-node
sudo /opt/tribe-node/pi/scripts/tor-guest.sh   # optional
```

### Dev test on PC (no AP)

```bash
cd pi
bash install-dev.sh
# open http://127.0.0.1:8080
```

Config: `/etc/tribe-node/config.json` (SSID, OLED, interfaces).

Web UI: **http://192.168.4.1** (same subnet as ESP nodes).

## Connecting ESP / D1 / Uno nodes

### A) ESP joins Pi (easiest)

1. Pi AP broadcasts **`TribeHub-Pi`** (open by default).
2. Flash any ESP env (`esp8266_az`, `zy_esp32`, `esp32dev`).
3. Phone/laptop joins `TribeHub-Pi` → browse `http://192.168.4.1`.
4. ESP can stay on its own AP for local UI, or you reconfigure ESP serial `uplink` to use Pi’s LAN if Pi shares internet via NAT.

### B) Pi uses ESP as uplink

ESP runs its normal AP (`TribeNode-XXXX`) with NAT to home WiFi. Pi joins as WiFi client:

```bash
sudo /opt/tribe-node/pi/scripts/uplink-via-esp.sh TribeNode-ABCD your_ap_pass
export TRIBE_UPLINK_IF=wlan1
sudo /opt/tribe-node/pi/scripts/setup-ap-network.sh
```

Pi still offers `TribeHub-Pi` on `wlan0`; internet exits through the ESP.

### C) Mesh discovery

UDP beacons on port **41414** (same as ESP firmware plan). Pi `/map` shows other Tribe SSIDs + mesh peers.

## Tor

| Mode | Use |
|------|-----|
| SOCKS5 | `127.0.0.1:9050` on the Pi |
| Guest transparent | `sudo scripts/tor-guest.sh` redirects AP TCP/DNS via Tor |

Pi 2 is slow for heavy Tor traffic — fine for light browsing; use **Tor Browser** on clients when possible.

This is **not** a full Tor relay by default (only client/proxy). Edit `/etc/tor/torrc` to add relay lines if you accept the operational risk.

## Services

```bash
sudo systemctl status tribe-node hostapd dnsmasq tor
sudo journalctl -u tribe-node -f
```

## OLED

Same 3-screen carousel as ESP: status, stats, Tor line. Disable in `config.json`: `"oled_enabled": false`.

## Development (off-Pi)

```bash
cd pi
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
export TRIBE_CONF_DIR=./config
python3 -m tribe_node
```

Runs on port 80 only as root, or set `"http_port": 8080` in config for testing.
