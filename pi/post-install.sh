#!/bin/bash
# Run after install.sh on the Pi: network, services, optional Tor guest routing.
set -euo pipefail

INSTALL_ROOT="${TRIBE_INSTALL_ROOT:-/opt/tribe-node}"
PI_DIR="$INSTALL_ROOT/pi"
SCRIPTS="$PI_DIR/scripts"

if [ "$(id -u)" -ne 0 ]; then
  echo "Run as root: sudo bash post-install.sh"
  exit 1
fi

echo "==> Apply config to hostapd/dnsmasq"
"$SCRIPTS/apply-config.sh"

echo "==> Unblock WiFi / stop NetworkManager on AP iface (if present)"
AP_IF=$(python3 -c "import json; print(json.load(open('/etc/tribe-node/config.json')).get('ap_iface','wlan0'))")
rfkill unblock wifi 2>/dev/null || true
if systemctl is-active --quiet NetworkManager 2>/dev/null; then
  nmcli dev set "$AP_IF" managed no 2>/dev/null || true
fi

echo "==> AP network address"
export TRIBE_AP_IF="$AP_IF"
export TRIBE_AP_IP=$(python3 -c "import json; print(json.load(open('/etc/tribe-node/config.json'))['ap_ip'])")
UPLINK=$(python3 -c "import json; c=json.load(open('/etc/tribe-node/config.json')); print(c.get('uplink_iface') or '')")
[ -n "$UPLINK" ] && export TRIBE_UPLINK_IF="$UPLINK"
"$SCRIPTS/setup-ap-network.sh"

echo "==> hostapd + dnsmasq"
systemctl unmask hostapd dnsmasq 2>/dev/null || true
systemctl enable hostapd dnsmasq
systemctl restart hostapd dnsmasq || {
  echo "WARN: hostapd/dnsmasq failed — check wlan0 exists and is not in use by wpa_supplicant"
  journalctl -u hostapd -n 20 --no-pager || true
}

echo "==> tribe-node"
systemctl restart tribe-node || systemctl start tribe-node
systemctl --no-pager status tribe-node --lines=5 || true

TOR_GUEST=$(python3 -c "import json; print(json.load(open('/etc/tribe-node/config.json')).get('tor_guest_proxy', False))")
if [ "$TOR_GUEST" = "True" ] || [ "$TOR_GUEST" = "true" ]; then
  echo "==> Tor guest proxy"
  systemctl restart tor
  sleep 2
  "$SCRIPTS/tor-guest.sh" || echo "WARN: tor-guest iptables failed (see README)"
fi

echo ""
echo "Done. Join WiFi and open http://${TRIBE_AP_IP:-192.168.4.1}/"
echo "Logs: journalctl -u tribe-node -f"
