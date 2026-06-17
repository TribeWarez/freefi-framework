#!/bin/bash
# Connect Pi STA (wlan1) to an ESP Tribe node AP for uplink.
# Usage: sudo ./uplink-via-esp.sh TribeNode-ABCD [password]
set -euo pipefail
STA_IF="${TRIBE_STA_IF:-wlan1}"
SSID="${1:?Usage: uplink-via-esp.sh <TribeNode-SSID> [pass]}"
PASS="${2:-}"

ip link set "$STA_IF" up
if [ -n "$PASS" ]; then
  wpa_passphrase "$SSID" "$PASS" > /tmp/tribe-wpa.conf
else
  cat > /tmp/tribe-wpa.conf <<EOF
network={
  ssid="$SSID"
  key_mgmt=NONE
}
EOF
fi
wpa_supplicant -B -i "$STA_IF" -c /tmp/tribe-wpa.conf
dhclient "$STA_IF" || dhcpcd "$STA_IF" || true
echo "STA $STA_IF joining $SSID — check: ip addr show $STA_IF"
