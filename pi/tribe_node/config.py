"""Tribe node config — Raspberry Pi hub (paths overridable via env)."""
import json
import os
from pathlib import Path

CONF_DIR = Path(os.environ.get("TRIBE_CONF_DIR", "/etc/tribe-node"))
DATA_DIR = Path(os.environ.get("TRIBE_DATA_DIR", "/var/lib/tribe-node"))
CONF_FILE = CONF_DIR / "config.json"

DEFAULTS = {
    "page_title": "Tribe Hub Pi",
    "ap_ssid": "TribeHub-Pi",
    "ap_pass": "",  # empty = open AP for ESP / guest join
    "ap_ip": "192.168.4.1",
    "ap_subnet": "192.168.4.0/24",
    "http_port": 80,
    "mesh_beacon_port": 41414,
    "tor_socks_port": 9050,
    "tor_guest_proxy": True,
    "privacy_redact": True,
    "oled_enabled": True,
    "oled_i2c_port": 1,
    "oled_sda": 2,
    "oled_scl": 3,
    "oled_interval_sec": 12,
    "uplink_iface": "",  # e.g. eth0 or wlan1 — empty = auto
    "ap_iface": "wlan0",
    "scan_iface": "",  # second USB WiFi for scan while AP up
}


def load_config():
    cfg = dict(DEFAULTS)
    if CONF_FILE.is_file():
        with open(CONF_FILE, encoding="utf-8") as f:
            cfg.update(json.load(f))
    return cfg


def save_config(cfg):
    CONF_DIR.mkdir(parents=True, exist_ok=True)
    with open(CONF_FILE, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)
