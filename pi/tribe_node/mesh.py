"""Mesh beacon + Tribe SSID detection (ESP nodes + Pi hub)."""
import json
import socket
import time
from typing import Any

BEACON_PORT = 41414
PEER_TTL = 120
MAX_PEERS = 16


def looks_like_tribe(ssid: str, self_ap: str) -> bool:
    if not ssid:
        return False
    if ssid == self_ap:
        return True
    s = ssid.lower()
    return (
        "|" in ssid
        or "repeater" in s
        or "tribe" in s
        or ssid.startswith("TribeNode")
        or ssid.startswith("TribeHub")
    )


class MeshRegistry:
    def __init__(self, node_id: str, ap_ssid: str):
        self.node_id = node_id
        self.ap_ssid = ap_ssid
        self.peers: dict[str, dict[str, Any]] = {}

    def prune(self):
        now = time.time()
        for k in list(self.peers.keys()):
            if now - self.peers[k].get("ts", 0) > PEER_TTL:
                del self.peers[k]

    def ingest_beacon(self, data: bytes, addr):
        try:
            j = json.loads(data.decode())
        except (json.JSONDecodeError, UnicodeDecodeError):
            return
        if j.get("id") == self.node_id:
            return
        j["ts"] = time.time()
        j["addr"] = addr[0]
        self.peers[j.get("id", addr[0])] = j
        while len(self.peers) > MAX_PEERS:
            oldest = min(self.peers, key=lambda k: self.peers[k].get("ts", 0))
            del self.peers[oldest]

    def build_beacon(self, clients: int, nat: bool, title: str) -> bytes:
        return json.dumps(
            {
                "id": self.node_id,
                "ssid": self.ap_ssid,
                "clients": clients,
                "nat": nat,
                "title": title,
                "role": "pi-hub",
                "tor": True,
            }
        ).encode()

    def peer_nodes_for_map(self):
        self.prune()
        out = []
        for i, (pid, p) in enumerate(self.peers.items()):
            out.append(
                {
                    "kind": "mesh",
                    "name": p.get("ssid", pid),
                    "rssi": -55,
                    "range": "~10m",
                    "angle": (hash(pid) % 360),
                    "tribe": 1,
                }
            )
        return out
