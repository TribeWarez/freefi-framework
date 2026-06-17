// TFT_eSPI project setup: 3.5" Mcufriend-class shield on ESP32 Uno (D1 R32 / TTGO).
// Pins match Bodmer Setup14_ILI9341_Parallel.h and MCUFRIEND mcufriend_shield.h (ESP32).
//
// Shield maps LCD_CS/LCD_RS/LCD_RST to ESP32 input-only GPIO 34/35/36 — add 3 jumpers:
//   shield A3 (CS)  -> ESP32 GPIO 33
//   shield A2 (RS)  -> ESP32 GPIO 15
//   shield A4 (RST) -> ESP32 GPIO 32
//
// After esp32_uno_lcd_id reports the controller, set ONE driver below (comment others).

#pragma once

#define USER_SETUP_ID 140

#define TFT_PARALLEL_8_BIT

// Placeholder until ID read; common 3.5" shields use ILI9486 or ILI9481.
#define ILI9486_DRIVER
// #define ILI9481_DRIVER
// #define ILI9341_DRIVER
// #define HX8357D_DRIVER

#define TFT_CS 33
#define TFT_DC 15
#define TFT_RST 32
#define TFT_WR 4
#define TFT_RD 2

#define TFT_D0 12
#define TFT_D1 13
#define TFT_D2 26
#define TFT_D3 25
#define TFT_D4 17
#define TFT_D5 16
#define TFT_D6 27
#define TFT_D7 14

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
