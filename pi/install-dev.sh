#!/bin/bash
# Dev install on PC (no hostapd): venv + run tribe-node on port 8080.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
python3 -m venv .venv
. .venv/bin/activate
pip install -q -r requirements.txt
export TRIBE_CONF_DIR="$ROOT/config"
export TRIBE_DATA_DIR="$ROOT/.data"
mkdir -p "$TRIBE_DATA_DIR"
if ! grep -q '"http_port"' "$TRIBE_CONF_DIR/config.json"; then
  true
fi
DEV_CFG="$TRIBE_DATA_DIR/config-dev.json"
if [ ! -f "$DEV_CFG" ]; then
  python3 -c "
import json, shutil
shutil.copy('$TRIBE_CONF_DIR/config.json', '$DEV_CFG')
c=json.load(open('$DEV_CFG'))
c['http_port']=8080
c['oled_enabled']=False
json.dump(c, open('$DEV_CFG','w'), indent=2)
"
fi
export TRIBE_CONF_DIR="$TRIBE_DATA_DIR"
cp "$DEV_CFG" "$TRIBE_CONF_DIR/config.json"
echo "Starting http://127.0.0.1:8080 (Ctrl+C to stop)"
python3 -m tribe_node
