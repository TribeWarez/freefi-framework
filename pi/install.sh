#!/bin/bash
# Tribe Pi hub installer — Raspberry Pi 2/3/4, 32-bit Raspbian Lite (armhf).
# Run on the Pi:  sudo bash install.sh && sudo bash post-install.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
INSTALL_ROOT="/opt/tribe-node"
CONF_DIR="/etc/tribe-node"

if [ "$(id -u)" -ne 0 ]; then
  echo "Run as root: sudo bash install.sh"
  exit 1
fi

if [ -f /proc/device-tree/model ]; then
  echo "Device: $(tr -d '\0' </proc/device-tree/model)"
else
  echo "WARN: not a Raspberry Pi — hostapd AP may not work; use install-dev.sh on PC"
fi

echo "==> Packages"
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  python3 python3-pip python3-venv \
  tor hostapd dnsmasq iptables \
  iw wireless-tools wpasupplicant \
  i2c-tools python3-smbus rsync \
  2>/dev/null || DEBIAN_FRONTEND=noninteractive apt-get install -y \
  python3 python3-pip python3-venv \
  tor hostapd dnsmasq iptables \
  iw wireless-tools wpasupplicant dhcpcd5 \
  i2c-tools python3-smbus rsync

echo "==> Install tree"
mkdir -p "$INSTALL_ROOT" "$CONF_DIR" /var/lib/tribe-node
rsync -a --delete "$ROOT/" "$INSTALL_ROOT/pi/"
chmod +x "$INSTALL_ROOT/pi/"*.sh 2>/dev/null || true
chmod +x "$INSTALL_ROOT/pi/scripts/"*.sh

if [ ! -f "$CONF_DIR/config.json" ]; then
  cp "$ROOT/config/config.json" "$CONF_DIR/config.json"
fi

echo "==> Python deps"
if python3 -m pip install --help 2>&1 | grep -q break-system-packages; then
  python3 -m pip install --break-system-packages -r "$INSTALL_ROOT/pi/requirements.txt"
else
  python3 -m pip install -r "$INSTALL_ROOT/pi/requirements.txt"
fi

echo "==> hostapd / dnsmasq"
cp "$ROOT/config/hostapd.conf" /etc/hostapd/hostapd.conf
if [ -f /etc/default/hostapd ]; then
  sed -i 's|^#DAEMON_CONF=|DAEMON_CONF=|' /etc/default/hostapd 2>/dev/null || true
  grep -q '^DAEMON_CONF=' /etc/default/hostapd ||
    echo 'DAEMON_CONF="/etc/hostapd/hostapd.conf"' >> /etc/default/hostapd
fi
cp "$ROOT/config/dnsmasq.conf" /etc/dnsmasq.d/tribe-node.conf
"$INSTALL_ROOT/pi/scripts/apply-config.sh"

echo "==> Tor"
if [ -f /etc/tor/torrc ]; then
  grep -q 'TRIBE_NODE' /etc/tor/torrc || {
    echo "" >> /etc/tor/torrc
    echo "# TRIBE_NODE" >> /etc/tor/torrc
    cat "$ROOT/config/torrc.fragment" >> /etc/tor/torrc
  }
  systemctl enable tor
  systemctl restart tor || echo "WARN: tor restart failed"
fi

echo "==> systemd"
cp "$ROOT/systemd/tribe-node.service" /etc/systemd/system/
systemctl daemon-reload
systemctl enable tribe-node

echo "==> I2C for OLED"
for BOOTCFG in /boot/firmware/config.txt /boot/config.txt; do
  if [ -f "$BOOTCFG" ]; then
    grep -q '^dtparam=i2c_arm=on' "$BOOTCFG" || echo 'dtparam=i2c_arm=on' >> "$BOOTCFG"
    echo "I2C enabled in $BOOTCFG (reboot if first time)"
    break
  fi
done

echo ""
echo "Installed to $INSTALL_ROOT"
echo "Config: $CONF_DIR/config.json"
echo ""
echo "Finish setup on the Pi:"
echo "  sudo bash $INSTALL_ROOT/pi/post-install.sh"
echo ""
echo "Or step by step — see $INSTALL_ROOT/pi/README.md"
