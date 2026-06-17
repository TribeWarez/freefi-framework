// TFT_eSPI color test — run after esp32_uno_lcd_id and set driver in tft_e_spi_uno_setup.h.
// Upload: pio run -t upload -e esp32_uno_tft_id

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("TFT_eSPI — ESP32 Uno parallel shield"));
  Serial.println(F("Board must be ESP32-WROOM Uno (R32), NOT ESP8266MOD D1."));
  Serial.println(F("Set *_DRIVER in include/tft_e_spi_uno_setup.h from MCUFRIEND readID."));
  Serial.println();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  delay(200);
  tft.fillScreen(TFT_RED);
  delay(400);
  tft.fillScreen(TFT_GREEN);
  delay(400);
  tft.fillScreen(TFT_BLUE);
  delay(400);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("TFT_eSPI OK", 10, 10, 2);
  tft.drawString("Check driver in setup.h", 10, 40, 2);
  Serial.println(F("Color flash + label — wrong colors/garbage => change *_DRIVER."));
}

void loop() {
  delay(10000);
}
