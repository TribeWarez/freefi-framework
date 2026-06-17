"""Flask HTTP UI + mesh beacon — Raspberry Pi Tribe hub."""
import json
import os
import socket
import subprocess
import threading
import time
import uuid

from flask import Flask, request, render_template, Response, jsonify

from .chat import ChatRoom
from .config import load_config, save_config, CONF_DIR, DATA_DIR
from .mesh import MeshRegistry, BEACON_PORT
from .oled import OledCarousel
from .wifi_scan import scan_networks

app = Flask(__name__, template_folder="templates")
cfg = load_config()
chat = ChatRoom()
node_id = hex(uuid.getnode() & 0xFFFF)[2:].zfill(4)
mesh = MeshRegistry(f"pi-{node_id}", cfg["ap_ssid"])
oled = None
_beacon_sock = None


def _tor_running() -> bool:
    try:
        s = socket.create_connection(("127.0.0.1", int(cfg.get("tor_socks_port", 9050))), timeout=1)
        s.close()
        return True
    except OSError:
        return False


def _nat_active() -> bool:
    try:
        out = subprocess.check_output(["iptables", "-t", "nat", "-L", "-n"], text=True, timeout=5)
        return "MASQUERADE" in out or "REDIRECT" in out
    except (subprocess.SubprocessError, FileNotFoundError):
        return False


def _ap_clients() -> int:
    try:
        out = subprocess.check_output(["iw", "dev", cfg["ap_iface"], "station", "dump"], text=True, timeout=5)
        return out.count("Station ")
    except (subprocess.SubprocessError, FileNotFoundError):
        return 0


def oled_status():
    redact = cfg.get("privacy_redact", True)
    return {
        "ap_ssid": cfg["ap_ssid"],
        "uplink_line": "Uplink OK" if _nat_active() else "Uplink —",
        "nat_line": "NAT on" if _nat_active() else "NAT off",
        "tor_line": "Tor up" if _tor_running() else "Tor down",
        "clients": _ap_clients(),
        "mesh_peers": len(mesh.peers),
        "ip_line": cfg["ap_ip"],
        "tor_port": cfg.get("tor_socks_port", 9050),
    }


def _beacon_loop():
    global _beacon_sock
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", BEACON_PORT))
    sock.settimeout(1.0)
    _beacon_sock = sock
    bcast = (cfg["ap_ip"].rsplit(".", 1)[0] + ".255", BEACON_PORT)
    while True:
        try:
            payload = mesh.build_beacon(_ap_clients(), _nat_active(), cfg["page_title"])
            sock.sendto(payload, bcast)
            try:
                data, addr = sock.recvfrom(512)
                mesh.ingest_beacon(data, addr)
            except socket.timeout:
                pass
        except OSError:
            pass
        time.sleep(30)


@app.route("/")
def index():
    tor = _tor_running()
    body = (
        f'<div class="tor-banner">Tor guest proxy: <b>{"active" if tor else "offline"}</b> '
        f'· SOCKS 127.0.0.1:{cfg.get("tor_socks_port", 9050)}</div>'
        f"<p><b>Hotspot:</b> <code>{cfg['ap_ssid']}</code><br>"
        f"AP IP <code>{cfg['ap_ip']}</code><br>"
        f"Clients: {_ap_clients()} · Mesh peers: {len(mesh.peers)}</p>"
        f"<p><b>Uplink:</b> {'connected' if _nat_active() else 'check eth0/wlan1 or ESP uplink'}</p>"
        "<p>ESP8266 / ESP32 / D1 nodes: join this WiFi or set Pi STA to your <code>TribeNode-*</code> AP.</p>"
    )
    return render_template("layout.html", page_title=cfg["page_title"], ap_ssid=cfg["ap_ssid"], body=body)


@app.route("/tor")
def tor_info():
    body = (
        "<p><b>Tor on this Pi</b></p>"
        "<ul style='text-align:left'>"
        f"<li>SOCKS5 proxy: <code>127.0.0.1:{cfg.get('tor_socks_port', 9050)}</code></li>"
        "<li>AP guests: traffic routed via Tor when <code>tor-guest</code> iptables is enabled</li>"
        "<li>Configure: <code>/etc/tor/torrc</code> + <code>sudo /opt/tribe-node/scripts/tor-guest.sh</code></li>"
        "</ul>"
        "<p>Use Tor Browser on clients for best results; guest NAT-Tor is best-effort on Pi 2.</p>"
    )
    return render_template("layout.html", page_title=cfg["page_title"], ap_ssid=cfg["ap_ssid"], body=body)


@app.route("/chat")
def chat_page():
    body = (
        '<p class="chat-warn">Single-node IRC on this hub. ESP nodes have separate chat unless bridged.</p>'
        f'<p>Channel <code>{cfg["ap_ssid"]}</code></p>'
        '<div id="chatLog" class="chat-log"></div>'
        '<form id="chatForm"><input id="chatIn" maxlength="95" placeholder="message">'
        '<button type="submit">Send</button></form>'
        + _CHAT_JS
    )
    return render_template("layout.html", page_title=cfg["page_title"], ap_ssid=cfg["ap_ssid"], body=body)


@app.route("/chat/poll")
def chat_poll():
    since = int(request.args.get("since", 0))
    ua = request.headers.get("User-Agent", "")
    return chat.poll_json(since, ua)


@app.route("/chat/send", methods=["POST"])
def chat_send():
    ua = request.headers.get("User-Agent", "")
    msg = request.form.get("msg", "")
    mid = chat.send(ua, msg)
    if mid is None:
        return jsonify({"ok": 0}), 400
    return jsonify({"ok": 1, "id": mid})


@app.route("/map")
def map_page():
    body = (
        "<p>Radar: WiFi scan + mesh beacons from Tribe ESP nodes.</p>"
        '<canvas id="nodeMap" class="node-map" width="320" height="320"></canvas>'
        '<ul id="mapList"></ul><p id="mapMeta">…</p>'
        '<button type="button" id="mapRefresh">Rescan</button>'
        + _MAP_JS
    )
    return render_template("layout.html", page_title=cfg["page_title"], ap_ssid=cfg["ap_ssid"], body=body)


@app.route("/map/data")
def map_data():
    mesh.prune()
    nodes = mesh.peer_nodes_for_map()
    scan_if = cfg.get("scan_iface") or cfg.get("uplink_iface", "")
    if scan_if:
        nodes.extend(scan_networks(scan_if, cfg["ap_ssid"]))
    data = {
        "self": {"ssid": cfg["ap_ssid"]},
        "hasBle": False,
        "radio": "Pi hub · WiFi scan + mesh UDP",
        "ranges": [
            {"rssi": -45, "label": "~2m"},
            {"rssi": -60, "label": "~10m"},
            {"rssi": -75, "label": "~30m"},
            {"rssi": -88, "label": "100m+"},
        ],
        "nodes": nodes,
    }
    return Response(json.dumps(data), mimetype="application/json")


@app.route("/favicon.ico")
def favicon():
    svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
<rect width="32" height="32" rx="6" fill="#0d0612"/>
<circle cx="16" cy="16" r="3" fill="#f60"/>
<text x="16" y="28" text-anchor="middle" fill="#94f" font-size="5">TW</text></svg>"""
    return Response(svg, mimetype="image/svg+xml")


@app.route("/config")
@app.route("/settings")
def operator_blocked():
    body = (
        "<p>Operator config on Pi: edit <code>/etc/tribe-node/config.json</code> "
        "and run <code>sudo systemctl restart tribe-node</code>.</p>"
    )
    return render_template("layout.html", page_title=cfg["page_title"], ap_ssid=cfg["ap_ssid"], body=body)


_CHAT_JS = r"""<script>
(function(){
var log=document.getElementById('chatLog'),form=document.getElementById('chatForm'),input=document.getElementById('chatIn'),since=0;
function esc(s){var d=document.createElement('div');d.textContent=s;return d.innerHTML;}
function poll(){fetch('/chat/poll?since='+since).then(r=>r.json()).then(j=>{
(j.messages||[]).forEach(m=>{var el=document.createElement('div');el.className='chat-line';
el.innerHTML=m.sys?'*** '+esc(m.text):'&lt;'+esc(m.user)+'&gt; '+esc(m.text);log.appendChild(el);since=Math.max(since,m.id);});
log.scrollTop=log.scrollHeight;});}
form.onsubmit=function(e){e.preventDefault();var msg=input.value.trim();if(!msg)return;
fetch('/chat/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(msg)})
.then(()=>{input.value='';poll();});};
setInterval(poll,2000);poll();
})();
</script>"""

_MAP_JS = r"""<script>
(function(){
var cv=document.getElementById('nodeMap'),meta=document.getElementById('mapMeta');
function draw(data){var ctx=cv.getContext('2d'),w=cv.width,h=cv.height,cx=w/2,cy=h/2,R=140;
ctx.clearRect(0,0,w,h);ctx.fillStyle='#f60';ctx.beginPath();ctx.arc(cx,cy,5,0,7);ctx.fill();
(data.nodes||[]).forEach(n=>{var ang=(n.angle||0)*Math.PI/180,r=R*0.5;
ctx.fillStyle=n.tribe?'#f60':'#94f';ctx.beginPath();ctx.arc(cx+Math.cos(ang)*r,cy+Math.sin(ang)*r,4,0,7);ctx.fill();});
meta.textContent=(data.radio||'')+' · '+(data.nodes||[]).length+' blips';}
function load(){fetch('/map/data').then(r=>r.json()).then(draw);}
document.getElementById('mapRefresh').onclick=load;load();setInterval(load,15000);
})();
</script>"""


def main():
    global oled, cfg
    cfg = load_config()
    mesh.ap_ssid = cfg["ap_ssid"]
    CONF_DIR.mkdir(parents=True, exist_ok=True)
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    threading.Thread(target=_beacon_loop, daemon=True).start()
    oled = OledCarousel(cfg, oled_status)
    oled.start()
    port = int(cfg.get("http_port", 80))
    print(f"Tribe Pi hub http://{cfg['ap_ip']}:{port}  AP={cfg['ap_ssid']}", flush=True)
    app.run(host="0.0.0.0", port=port, threaded=True, use_reloader=False)
