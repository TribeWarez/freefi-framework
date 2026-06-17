#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"

#ifndef OLED_SDA
#define OLED_SDA 21
#endif
#ifndef OLED_SCL
#define OLED_SCL 22
#endif
#ifndef OLED_SDA_ALT
#define OLED_SDA_ALT OLED_SDA
#endif
#ifndef OLED_SCL_ALT
#define OLED_SCL_ALT OLED_SCL
#endif
#ifndef OLED_SDA_ALT2
#define OLED_SDA_ALT2 -1
#endif
#ifndef OLED_SCL_ALT2
#define OLED_SCL_ALT2 -1
#endif

static inline bool oledI2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static inline bool oledTryBus(int sda, int scl, uint8_t &foundAddr) {
  foundAddr = 0;
  Wire.begin(sda, scl);
  Wire.setClock(100000);
  delay(20);
  Serial.printf("I2C probe SDA=GPIO%d SCL=GPIO%d: ", sda, scl);
  const uint8_t addrs[] = {0x3C, 0x3D};
  bool any = false;
  for (uint8_t addr : addrs) {
    if (oledI2cProbe(addr)) {
      Serial.printf("0x%02X ", addr);
      any = true;
      foundAddr = addr;
    }
  }
  Serial.println(any ? "" : "(none)");
  Serial.flush();
  return foundAddr != 0;
}

#if defined(ESP8266_OLED_AUTODETECT) || defined(ESP32_OLED_AUTODETECT)

static inline SSD1306Wire *initOledAutodetect() {
  int pairs[3][2] = {{OLED_SDA, OLED_SCL},
                     {OLED_SDA_ALT, OLED_SCL_ALT},
                     {OLED_SDA_ALT2, OLED_SCL_ALT2}};
  uint8_t nPairs = 2;
  if (OLED_SDA_ALT2 >= 0 && OLED_SCL_ALT2 >= 0) {
    nPairs = 3;
  }
  uint8_t foundAddr = 0;

  for (uint8_t i = 0; i < nPairs; i++) {
    if (pairs[i][0] < 0 || pairs[i][1] < 0) {
      continue;
    }
    foundAddr = 0;
    if (!oledTryBus(pairs[i][0], pairs[i][1], foundAddr)) {
      continue;
    }
    auto *oled = new SSD1306Wire(foundAddr, pairs[i][0], pairs[i][1]);
    if (oled->init()) {
      oled->clear();
      oled->display();
      oled->setFont(ArialMT_Plain_10);
      Serial.printf("OLED OK @ 0x%02X on SDA=%d SCL=%d\n", foundAddr, pairs[i][0],
                    pairs[i][1]);
      return oled;
    }
    delete oled;
    Serial.printf("SSD1306 init failed @ 0x%02X SDA=%d SCL=%d\n", foundAddr,
                  pairs[i][0], pairs[i][1]);
  }

  Serial.println("OLED not found — check I2C wiring (3.3V, GND, addr 0x3C)");
  return nullptr;
}

#else

static SSD1306Wire display3c(0x3C, OLED_SDA, OLED_SCL);
static SSD1306Wire display3d(0x3D, OLED_SDA, OLED_SCL);

static inline SSD1306Wire *initOledAutodetect() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  if (display3c.init()) {
    display3c.clear();
    display3c.display();
    display3c.setFont(ArialMT_Plain_10);
    Serial.println("OLED OK @ 0x3C");
    return &display3c;
  }
  if (display3d.init()) {
    display3d.clear();
    display3d.display();
    display3d.setFont(ArialMT_Plain_10);
    Serial.println("OLED OK @ 0x3D");
    return &display3d;
  }
  Serial.println("OLED init failed");
  return nullptr;
}

#endif

static inline void oledRandomSeed() {
#if defined(ESP8266)
  randomSeed(ESP.random());
#else
#include <esp_random.h>
  randomSeed(esp_random());
#endif
}
