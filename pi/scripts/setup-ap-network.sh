#!/bin/bash
# Static AP IP on wlan0 (run after hostapd enabled).
set -euo pipefail
AP_IF="${TRIBE_AP_IF:-wlan0}"
AP_IP="${TRIBE_AP_IP:-192.168.4.1}"

ip link set "$AP_IF" up
ip addr flush dev "$AP_IF"
ip addr add "${AP_IP}/24" dev "$AP_IF"

# NAT uplink (eth0 or wlan1) — adjust TRIBE_UPLINK_IF
UPLINK="${TRIBE_UPLINK_IF:-eth0}"
if ip link show "$UPLINK" &>/dev/null; then
  sysctl -w net.ipv4.ip_forward=1
  iptables -t nat -C POSTROUTING -o "$UPLINK" -j MASQUERADE 2>/dev/null ||
    iptables -t nat -A POSTROUTING -o "$UPLINK" -j MASQUERADE
  echo "NAT MASQUERADE via $UPLINK"
fi

echo "AP $AP_IF = $AP_IP"
