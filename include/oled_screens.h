#pragma once

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "SSD1306Wire.h"
#include "oled_init.h"

#ifndef OLED_SCREEN_INTERVAL_MS
#define OLED_SCREEN_INTERVAL_MS 12444
#endif

static constexpr uint8_t OLED_SCREEN_COUNT = 3;
static constexpr uint32_t OLED_OWL_FRAME_MS = 90;
static constexpr uint32_t OLED_BAR_ANIM_MS = 110;
static constexpr uint8_t RIGHT_BAR_COLS = 5;
static constexpr int16_t RIGHT_BAR_X0 = 110;
static constexpr int16_t RIGHT_BAR_SEP_X = 106;
static constexpr uint8_t RIGHT_BAR_ROW_H = 10;

static const char MATRIX_CHARS[] = "TWZ01アイウエオ";

struct OledScreenState {
  uint8_t index = 0;
  uint32_t lastSwitchMs = 0;
  uint32_t lastStatsMs = 0;
  uint32_t lastOwlFrameMs = 0;
  uint32_t lastBarAnimMs = 0;
  uint32_t owlFrame = 0;
  uint32_t rightBarFrame = 0;
  uint8_t matrixColY[8] = {0};
  uint8_t rightBarHead[RIGHT_BAR_COLS] = {0};
  uint8_t rightBarChar[RIGHT_BAR_COLS] = {0};
  uint8_t rightBarSpeed[RIGHT_BAR_COLS] = {0};
};

struct OledMetrics {
  bool naptEnabled = false;
  bool privacyRedact = true;
  const char *apSsid = "";
  const String *uplinkSsid = nullptr;
};

static inline const char *linkSpeedLabel(int rssi) {
  if (rssi >= -50) {
    return "~72 Mbps";
  }
  if (rssi >= -60) {
    return "~54 Mbps";
  }
  if (rssi >= -70) {
    return "~24 Mbps";
  }
  if (rssi >= -80) {
    return "~11 Mbps";
  }
  return "~1 Mbps";
}

static inline void resetRightMatrixBar(OledScreenState &st) {
  st.rightBarFrame = 0;
  oledRandomSeed();
  for (uint8_t c = 0; c < RIGHT_BAR_COLS; c++) {
    st.rightBarHead[c] = random(0, 7);
    st.rightBarChar[c] = random(0, sizeof(MATRIX_CHARS) - 1);
    st.rightBarSpeed[c] = random(1, 4);
  }
}

static inline void drawRightMatrixBar(SSD1306Wire &d, OledScreenState &st, uint8_t screenSeed) {
  d.setColor(WHITE);
  d.drawVerticalLine(RIGHT_BAR_SEP_X, 0, 64);

  d.setFont(ArialMT_Plain_10);
  d.setTextAlignment(TEXT_ALIGN_LEFT);

  const uint8_t charCount = sizeof(MATRIX_CHARS) - 1;

  for (uint8_t c = 0; c < RIGHT_BAR_COLS; c++) {
    const int16_t x = RIGHT_BAR_X0 + c * 4;

    if (((st.rightBarFrame + c + screenSeed) % 11) == 0) {
      st.rightBarChar[c] = random(0, charCount);
    }
    if (((st.rightBarFrame + c) % 19) == 0) {
      st.rightBarSpeed[c] = random(1, 4);
    }
    if (((st.rightBarFrame + c) % 23) == 0) {
      st.rightBarHead[c] = random(0, 7);
    }

    st.rightBarHead[c] =
        (st.rightBarHead[c] + st.rightBarSpeed[c] + (c & 1)) % 7;

    for (uint8_t trail = 0; trail < 6; trail++) {
      const int16_t row = static_cast<int16_t>(st.rightBarHead[c]) - trail;
      if (row < 0 || row > 6) {
        continue;
      }
      const int16_t y = row * RIGHT_BAR_ROW_H;
      if (y > 56) {
        continue;
      }
      const uint8_t ci =
          (st.rightBarChar[c] + trail + st.rightBarFrame + screenSeed) % charCount;
      char ch[2] = {MATRIX_CHARS[ci], 0};
      d.drawString(x, y, ch);
    }
  }

  st.rightBarFrame++;
}

static inline void drawOwlSprite(SSD1306Wire &d, int16_t ox, int16_t oy, bool eyesOpen) {
  d.setColor(WHITE);
  d.fillTriangle(ox + 6, oy + 2, ox + 11, oy + 12, ox + 2, oy + 12);
  d.fillTriangle(ox + 26, oy + 2, ox + 21, oy + 12, ox + 30, oy + 12);
  d.fillCircle(ox + 16, oy + 16, 13);
  d.setColor(BLACK);
  if (eyesOpen) {
    d.fillCircle(ox + 10, oy + 13, 4);
    d.fillCircle(ox + 22, oy + 13, 4);
    d.setColor(WHITE);
    d.fillCircle(ox + 11, oy + 13, 1);
    d.fillCircle(ox + 23, oy + 13, 1);
  } else {
    d.drawHorizontalLine(ox + 7, oy + 14, 18);
  }
  d.setColor(WHITE);
  d.fillTriangle(ox + 16, oy + 19, ox + 12, oy + 24, ox + 20, oy + 24);
}

static inline void drawScreenStatus(SSD1306Wire &d, OledScreenState &st, const OledMetrics &m) {
  d.setFont(ArialMT_Plain_10);
  d.setTextAlignment(TEXT_ALIGN_LEFT);
  d.clear();
  d.drawString(0, 0, "STATUS 1/3");
  d.drawString(0, 12, String(m.apSsid).substring(0, 15));
  if (WiFi.status() == WL_CONNECTED) {
    d.drawString(0, 26, m.privacyRedact ? "Uplink OK" : WiFi.SSID().substring(0, 15));
    d.drawString(0, 38, WiFi.localIP().toString());
    d.drawString(0, 50, m.naptEnabled ? "NAT ON" : "NAT wait");
  } else if (m.uplinkSsid != nullptr && m.uplinkSsid->length() > 0) {
    d.drawString(0, 26, "Uplink...");
    d.drawString(0, 38, m.privacyRedact ? "192.168.4.1" : m.uplinkSsid->substring(0, 15));
    d.drawString(0, 50, "192.168.4.1");
  } else {
    d.drawString(0, 26, "Set uplink:");
    d.drawString(0, 38, "USB serial");
    d.drawString(0, 50, "192.168.4.1");
  }
  drawRightMatrixBar(d, st, 0);
  d.display();
}

static inline void drawScreenStats(SSD1306Wire &d, OledScreenState &st, const OledMetrics &m) {
  const uint8_t clients = WiFi.softAPgetStationNum();
  const uint32_t upSec = millis() / 1000;
  const uint32_t heapK = ESP.getFreeHeap() / 1024;
  const int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  const uint8_t ch = WiFi.channel();

  d.setFont(ArialMT_Plain_10);
  d.setTextAlignment(TEXT_ALIGN_LEFT);
  d.clear();
  d.drawString(0, 0, "STATS 2/3");
  d.drawString(0, 14, "Cli:" + String(clients));
  if (WiFi.status() == WL_CONNECTED) {
    d.drawString(0, 26, String(rssi) + "dBm " + String(linkSpeedLabel(rssi)));
    d.drawString(0, 38, "Ch" + String(ch) + " Up:" + String(upSec) + "s");
    d.drawString(0, 50, "NAT" + String(m.naptEnabled ? "+" : "-") + " H:" + String(heapK) + "K");
  } else {
    d.drawString(0, 26, "Uplink offline");
    d.drawString(0, 38, "AP Ch:" + String(ch));
    d.drawString(0, 50, "Up:" + String(upSec) + "s H:" + String(heapK) + "K");
  }
  drawRightMatrixBar(d, st, 17);
  d.display();
}

static inline void drawScreenMatrixOwl(SSD1306Wire &d, OledScreenState &st) {
  d.setFont(ArialMT_Plain_10);
  d.setTextAlignment(TEXT_ALIGN_CENTER);
  d.clear();

  const String title = "TRIBEWAREZ";
  d.drawString(64, 0, title);
  d.setTextAlignment(TEXT_ALIGN_LEFT);
  d.drawString(40, 54, "matrix Owl");

  const int16_t owlX = 36;
  const int16_t owlY = 12;
  const bool eyesOpen = (st.owlFrame / 5) % 2 == 0;
  drawOwlSprite(d, owlX, owlY, eyesOpen);

  for (uint8_t c = 0; c < 8; c++) {
    if (st.matrixColY[c] == 0 || (st.owlFrame + c) % 17 == 0) {
      st.matrixColY[c] = random(0, 8);
    }
    const int16_t colX = (c < 4) ? c * 3 : 128 - (8 - c) * 3 - 6;
    for (uint8_t drop = 0; drop < 3; drop++) {
      int16_t y = static_cast<int16_t>(st.matrixColY[c] * 8 + drop * 10 + (st.owlFrame % 5));
      if (y > 63) {
        continue;
      }
      char ch[2] = {MATRIX_CHARS[random(0, sizeof(MATRIX_CHARS) - 1)], 0};
      d.setFont(ArialMT_Plain_10);
      d.drawString(colX, y, ch);
    }
    st.matrixColY[c] = (st.matrixColY[c] + 1) % 8;
  }

  d.display();
}

static inline void oledScreensTick(SSD1306Wire *display, OledScreenState &st, const OledMetrics &m) {
  if (display == nullptr) {
    return;
  }

  const uint32_t now = millis();

  if (now - st.lastSwitchMs >= OLED_SCREEN_INTERVAL_MS) {
    st.lastSwitchMs = now;
    st.index = (st.index + 1) % OLED_SCREEN_COUNT;
    st.owlFrame = 0;
    st.lastOwlFrameMs = now;
    st.lastStatsMs = now;
    st.lastBarAnimMs = now;
    resetRightMatrixBar(st);
  }

  if (st.index == 0) {
    if (now - st.lastBarAnimMs >= OLED_BAR_ANIM_MS) {
      st.lastBarAnimMs = now;
      drawScreenStatus(*display, st, m);
    }
    return;
  }

  if (st.index == 1) {
    if (now - st.lastBarAnimMs >= OLED_BAR_ANIM_MS) {
      st.lastBarAnimMs = now;
      drawScreenStats(*display, st, m);
    }
    return;
  }

  if (now - st.lastOwlFrameMs >= OLED_OWL_FRAME_MS) {
    st.lastOwlFrameMs = now;
    st.owlFrame++;
    drawScreenMatrixOwl(*display, st);
  }
}

static inline void oledScreensForceDraw(SSD1306Wire *display, OledScreenState &st,
                                        const OledMetrics &m) {
  if (display == nullptr) {
    return;
  }
  st.lastSwitchMs = millis();
  st.lastStatsMs = 0;
  st.lastOwlFrameMs = 0;
  st.lastBarAnimMs = 0;
  resetRightMatrixBar(st);
  switch (st.index) {
    case 0:
      drawScreenStatus(*display, st, m);
      break;
    case 1:
      drawScreenStats(*display, st, m);
      break;
    default:
      drawScreenMatrixOwl(*display, st);
      break;
  }
}
