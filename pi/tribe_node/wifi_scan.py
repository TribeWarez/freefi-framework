"""WiFi scan via iw (optional second interface on Pi)."""
import re
import subprocess
from typing import List, Dict

from .mesh import looks_like_tribe


def scan_networks(iface: str, self_ap: str) -> List[Dict]:
    if not iface:
        return []
    try:
        subprocess.run(
            ["iw", "dev", iface, "scan"],
            capture_output=True,
            timeout=25,
            check=False,
        )
        out = subprocess.check_output(
            ["iw", "dev", iface, "scan"],
            text=True,
            timeout=25,
        )
    except (subprocess.SubprocessError, FileNotFoundError):
        return []
    return _parse_iw_scan(out, self_ap)


def _parse_iw_scan(text: str, self_ap: str) -> List[Dict]:
    results = []
    bss_blocks = re.split(r"(?=BSS )", text)
    for block in bss_blocks:
        m_ssid = re.search(r"SSID: (.+)", block)
        m_sig = re.search(r"signal: ([-\d.]+)", block)
        if not m_ssid:
            continue
        ssid = m_ssid.group(1).strip()
        if not ssid:
            continue
        rssi = int(float(m_sig.group(1))) if m_sig else -80
        results.append(
            {
                "kind": "wifi",
                "name": ssid,
                "rssi": rssi,
                "secure": 1 if "RSN:" in block or "WPA:" in block else 0,
                "tribe": 1 if looks_like_tribe(ssid, self_ap) else 0,
                "angle": hash(ssid) % 360,
                "range": _rssi_label(rssi),
            }
        )
    return results[:24]


def _rssi_label(rssi: int) -> str:
    if rssi >= -45:
        return "~2m"
    if rssi >= -60:
        return "~10m"
    if rssi >= -75:
        return "~30m"
    return "100m+"
