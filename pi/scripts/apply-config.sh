#!/bin/bash
# Sync /etc/tribe-node/config.json -> hostapd + dnsmasq snippets.
set -euo pipefail
CONF="${TRIBE_CONF_DIR:-/etc/tribe-node}/config.json"
if [ ! -f "$CONF" ]; then
  echo "Missing $CONF"
  exit 1
fi
AP_SSID=$(python3 -c "import json; print(json.load(open('$CONF'))['ap_ssid'])")
AP_IP=$(python3 -c "import json; print(json.load(open('$CONF'))['ap_ip'])")
AP_IF=$(python3 -c "import json; print(json.load(open('$CONF')).get('ap_iface','wlan0'))")
AP_PASS=$(python3 -c "import json; print(json.load(open('$CONF')).get('ap_pass',''))")

if [ -f /etc/hostapd/hostapd.conf ]; then
  sed -i "s/^interface=.*/interface=$AP_IF/" /etc/hostapd/hostapd.conf
  sed -i "s/^ssid=.*/ssid=$AP_SSID/" /etc/hostapd/hostapd.conf
  if [ -n "$AP_PASS" ]; then
    grep -q '^wpa=' /etc/hostapd/hostapd.conf || cat >> /etc/hostapd/hostapd.conf <<EOF
wpa=2
wpa_passphrase=$AP_PASS
wpa_key_mgmt=WPA-PSK
EOF
  fi
fi
if [ -f /etc/dnsmasq.d/tribe-node.conf ]; then
  sed -i "s/^interface=.*/interface=$AP_IF/" /etc/dnsmasq.d/tribe-node.conf
  BASE="${AP_IP%.*}"
  sed -i "s|^dhcp-range=.*|dhcp-range=${BASE}.10,${BASE}.50,255.255.255.0,12h|" /etc/dnsmasq.d/tribe-node.conf
  sed -i "s|^dhcp-option=3,.*|dhcp-option=3,$AP_IP|" /etc/dnsmasq.d/tribe-node.conf
  sed -i "s|^dhcp-option=6,.*|dhcp-option=6,$AP_IP|" /etc/dnsmasq.d/tribe-node.conf
fi
echo "Applied AP_SSID=$AP_SSID AP_IP=$AP_IP iface=$AP_IF"
