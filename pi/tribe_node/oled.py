"""0.96\" SSD1306 OLED status carousel (I2C, same wiring as ESP nodes)."""
import threading
import time


class OledCarousel:
    def __init__(self, cfg, status_fn):
        self.cfg = cfg
        self.status_fn = status_fn
        self.device = None
        self._stop = threading.Event()
        self._thread = None

    def start(self):
        if not self.cfg.get("oled_enabled", True):
            return
        try:
            from luma.core.interface.serial import i2c
            from luma.oled.device import ssd1306

            port = int(self.cfg.get("oled_i2c_port", 1))
            self.device = ssd1306(
                i2c(
                    port=port,
                    address=0x3C,
                    gpio_mode="bcm",
                )
            )
        except Exception as e:
            print(f"OLED init skipped: {e}", flush=True)
            return
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop.set()

    def _loop(self):
        from PIL import Image, ImageDraw, ImageFont

        screen = 0
        interval = float(self.cfg.get("oled_interval_sec", 12))
        while not self._stop.is_set():
            st = self.status_fn()
            img = Image.new("1", self.device.size)
            draw = ImageDraw.Draw(img)
            try:
                font = ImageFont.load_default()
            except Exception:
                font = None
            lines = self._lines_for_screen(screen, st)
            y = 0
            for line in lines[:6]:
                draw.text((0, y), line[:21], fill=255, font=font)
                y += 10
            self.device.display(img)
            screen = (screen + 1) % 3
            self._stop.wait(interval)

    def _lines_for_screen(self, idx: int, st: dict):
        if idx == 0:
            return [
                "TRIBE HUB Pi",
                st.get("ap_ssid", "")[:21],
                st.get("uplink_line", "")[:21],
                st.get("nat_line", "")[:21],
                st.get("tor_line", "")[:21],
            ]
        if idx == 1:
            return [
                "STATS",
                f"Clients: {st.get('clients', 0)}",
                f"Mesh: {st.get('mesh_peers', 0)}",
                st.get("ip_line", "")[:21],
            ]
        return ["TRIBEWAREZ", "Tor SOCKS", f":{st.get('tor_port', 9050)}", "docs.tribewarez.com"]
