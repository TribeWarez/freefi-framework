#!/bin/bash
# Remove Tor guest iptables rules (best-effort).
set -euo pipefail
AP_IF="${TRIBE_AP_IF:-wlan0}"
TOR_TRANS=9040
TOR_DNS=5353

iptables -t nat -D PREROUTING -i "$AP_IF" -p udp --dport 53 -j REDIRECT --to-ports "$TOR_DNS" 2>/dev/null || true
iptables -t nat -D PREROUTING -i "$AP_IF" -p tcp --syn -j REDIRECT --to-ports "$TOR_TRANS" 2>/dev/null || true
iptables -D FORWARD -i "$AP_IF" -o lo -j ACCEPT 2>/dev/null || true
echo "Tor guest routing rules removed from $AP_IF"
