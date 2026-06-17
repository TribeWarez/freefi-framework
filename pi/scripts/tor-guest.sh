#!/bin/bash
# Route Tribe AP clients (192.168.4.0/24) through Tor transparent proxy.
# Requires tor TransPort 9040 + DNSPort 5353 (see config/torrc.fragment).
set -euo pipefail

AP_IF="${TRIBE_AP_IF:-wlan0}"
AP_NET="${TRIBE_AP_NET:-192.168.4.0/24}"
TOR_TRANS=9040
TOR_DNS=5353

if ! systemctl is-active --quiet tor; then
  echo "Start tor first: sudo systemctl start tor"
  exit 1
fi

# DNS to Tor for AP subnet
iptables -t nat -C PREROUTING -i "$AP_IF" -p udp --dport 53 -j REDIRECT --to-ports "$TOR_DNS" 2>/dev/null ||
  iptables -t nat -A PREROUTING -i "$AP_IF" -p udp --dport 53 -j REDIRECT --to-ports "$TOR_DNS"

# TCP through Tor TransPort
iptables -t nat -C PREROUTING -i "$AP_IF" -p tcp --syn -j REDIRECT --to-ports "$TOR_TRANS" 2>/dev/null ||
  iptables -t nat -A PREROUTING -i "$AP_IF" -p tcp --syn -j REDIRECT --to-ports "$TOR_TRANS"

# Allow established
iptables -C FORWARD -i "$AP_IF" -o lo -j ACCEPT 2>/dev/null ||
  iptables -A FORWARD -i "$AP_IF" -o lo -j ACCEPT

echo "Tor guest routing enabled on $AP_IF ($AP_NET)"
