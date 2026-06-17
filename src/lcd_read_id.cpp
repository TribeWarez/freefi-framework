// MCUFRIEND_kbv ID + support check for 3.5" Arduino Uno parallel TFT shield on ESP32 Uno.
// Upload: pio run -t upload -e esp32_uno_lcd_id
// Serial monitor 115200 — copy tft.readID() line into tft_e_spi_uno_setup.h driver choice.

#include <Arduino.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("MCUFRIEND_kbv — ESP32 Uno + 3.5\" Mcufriend shield"));
  Serial.println(F("Requires ESP32-WROOM in Uno form (D1 R32), NOT ESP8266MOD."));
  Serial.println(F("GPIO 33/15/32 jumpers for CS/RS/RST if shield uses A3/A2/A4."));
  Serial.println();

  uint16_t id = tft.readID();
  Serial.print(F("tft.readID() = 0x"));
  Serial.println(id, HEX);
  Serial.println();

  if (id == 0xD3D3) {
    Serial.println(F("Write-only shield — trying forced ID 0x9481"));
    tft.begin(0x9481);
  } else {
    tft.begin(id);
  }

  if (tft.width() == 0) {
    Serial.println(F("ID not supported in this MCUFRIEND_kbv build."));
    Serial.println(F("Run: pio run -t upload -e esp32_uno_lcd_id (LCD_ID_readreg) or"));
    Serial.println(F("enable SUPPORT_* in MCUFRIEND_kbv.cpp per library FAQ."));
    Serial.println(F("Then retry esp32_uno_tft_id with matching TFT_eSPI driver."));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(F("Display: "));
  Serial.print(tft.width());
  Serial.print(F(" x "));
  Serial.println(tft.height());
  Serial.println();
  Serial.println(F("Driver hints for include/tft_e_spi_uno_setup.h:"));
  Serial.println(F("  0x9341 -> ILI9341_DRIVER"));
  Serial.println(F("  0x9481 -> ILI9481_DRIVER"));
  Serial.println(F("  0x9486 -> ILI9486_DRIVER"));
  Serial.println(F("  0x8357 -> HX8357D_DRIVER"));
  Serial.println(F("Next: pio run -t upload -e esp32_uno_tft_id"));
}

void loop() {
  delay(5000);
}
