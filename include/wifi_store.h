#pragma once

#include <Arduino.h>
#include "config.h"
#include "credential_crypto.h"

static constexpr uint8_t WIFI_SSID_MAX = 64;
static constexpr uint8_t WIFI_PASS_MAX = 64;
static constexpr size_t WIFI_ENC_SSID_BLOB = 1 + CRED_NONCE_LEN + WIFI_SSID_MAX;
static constexpr size_t WIFI_ENC_PASS_BLOB = 1 + CRED_NONCE_LEN + WIFI_PASS_MAX;

#if defined(ESP8266)

#include <EEPROM.h>

static constexpr uint16_t WIFI_EEPROM_SIZE = 512;

static inline void wifiReadLegacyPlain(uint16_t base, String &ssid, String &pass) {
  char buf[WIFI_SSID_MAX + 1] = {0};
  for (uint8_t i = 0; i < WIFI_SSID_MAX; i++) {
    buf[i] = static_cast<char>(EEPROM.read(base + i));
  }
  ssid = String(buf);
  for (uint8_t i = 0; i < WIFI_PASS_MAX; i++) {
    buf[i] = static_cast<char>(EEPROM.read(base + WIFI_SSID_MAX + i));
  }
  pass = String(buf);
}

static inline void loadWifiConfig(String &ssid, String &pass) {
  ssid = UPLINK_SSID;
  pass = UPLINK_PASS;
  EEPROM.begin(WIFI_EEPROM_SIZE);
  const uint16_t magic = EEPROM.read(0) | (static_cast<uint16_t>(EEPROM.read(1)) << 8);
  if (magic == CRED_MAGIC_WIFI_LEGACY) {
    wifiReadLegacyPlain(2, ssid, pass);
    ssid = credSanitizeString(ssid);
    pass = credSanitizeString(pass);
    EEPROM.end();
    return;
  }
  if (magic != CRED_MAGIC_ENCRYPTED) {
    EEPROM.end();
    return;
  }
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t blob[WIFI_ENC_SSID_BLOB];
  for (size_t i = 0; i < WIFI_ENC_SSID_BLOB; i++) {
    blob[i] = EEPROM.read(2 + i);
  }
  char buf[WIFI_SSID_MAX + 1];
  if (credDecryptField(blob, WIFI_ENC_SSID_BLOB, WIFI_SSID_MAX, buf, sizeof(buf), key)) {
    ssid = String(buf);
  }
  for (size_t i = 0; i < WIFI_ENC_PASS_BLOB; i++) {
    blob[i] = EEPROM.read(2 + WIFI_ENC_SSID_BLOB + i);
  }
  if (credDecryptField(blob, WIFI_ENC_PASS_BLOB, WIFI_PASS_MAX, buf, sizeof(buf), key)) {
    pass = String(buf);
  }
  ssid = credSanitizeString(ssid);
  pass = credSanitizeString(pass);
  if (!credStringIsValid(ssid)) {
    ssid = "";
  }
  if (!credStringIsValid(pass)) {
    pass = "";
  }
  EEPROM.end();
}

static inline void saveWifiConfig(const String &ssid, const String &pass) {
  const String s = credSanitizeString(ssid);
  const String p = credSanitizeString(pass);
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t ssidBlob[WIFI_ENC_SSID_BLOB];
  uint8_t passBlob[WIFI_ENC_PASS_BLOB];
  credEncryptField(s.c_str(), WIFI_SSID_MAX, ssidBlob, sizeof(ssidBlob), key);
  credEncryptField(p.c_str(), WIFI_PASS_MAX, passBlob, sizeof(passBlob), key);

  EEPROM.begin(WIFI_EEPROM_SIZE);
  EEPROM.write(0, CRED_MAGIC_ENCRYPTED & 0xFF);
  EEPROM.write(1, (CRED_MAGIC_ENCRYPTED >> 8) & 0xFF);
  for (size_t i = 0; i < WIFI_ENC_SSID_BLOB; i++) {
    EEPROM.write(2 + i, ssidBlob[i]);
  }
  for (size_t i = 0; i < WIFI_ENC_PASS_BLOB; i++) {
    EEPROM.write(2 + WIFI_ENC_SSID_BLOB + i, passBlob[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

static inline void clearWifiConfig() {
  saveWifiConfig("", "");
}

#else

#include <Preferences.h>

static inline void loadWifiConfig(String &ssid, String &pass) {
  ssid = UPLINK_SSID;
  pass = UPLINK_PASS;
  Preferences prefs;
  if (!prefs.begin("repeater", true)) {
    return;
  }
  const uint16_t magic = static_cast<uint16_t>(prefs.getUShort("magic", 0));
  if (magic == CRED_MAGIC_WIFI_LEGACY || magic == 0) {
    ssid = prefs.getString("ssid", ssid);
    pass = prefs.getString("pass", pass);
    prefs.end();
    return;
  }
  if (magic != CRED_MAGIC_ENCRYPTED) {
    prefs.end();
    return;
  }
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  size_t len = prefs.getBytesLength("essid");
  if (len == WIFI_ENC_SSID_BLOB) {
    uint8_t blob[WIFI_ENC_SSID_BLOB];
    prefs.getBytes("essid", blob, len);
    char buf[WIFI_SSID_MAX + 1];
    if (credDecryptField(blob, len, WIFI_SSID_MAX, buf, sizeof(buf), key)) {
      ssid = String(buf);
    }
  }
  len = prefs.getBytesLength("epass");
  if (len == WIFI_ENC_PASS_BLOB) {
    uint8_t blob[WIFI_ENC_PASS_BLOB];
    prefs.getBytes("epass", blob, len);
    char buf[WIFI_PASS_MAX + 1];
    if (credDecryptField(blob, len, WIFI_PASS_MAX, buf, sizeof(buf), key)) {
      pass = String(buf);
    }
  }
  ssid = credSanitizeString(ssid);
  pass = credSanitizeString(pass);
  if (!credStringIsValid(ssid)) {
    ssid = "";
  }
  if (!credStringIsValid(pass)) {
    pass = "";
  }
  prefs.end();
}

static inline void saveWifiConfig(const String &ssid, const String &pass) {
  String s = credSanitizeString(ssid);
  String p = credSanitizeString(pass);
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t ssidBlob[WIFI_ENC_SSID_BLOB];
  uint8_t passBlob[WIFI_ENC_PASS_BLOB];
  credEncryptField(s.c_str(), WIFI_SSID_MAX, ssidBlob, sizeof(ssidBlob), key);
  credEncryptField(p.c_str(), WIFI_PASS_MAX, passBlob, sizeof(passBlob), key);

  Preferences prefs;
  prefs.begin("repeater", false);
  prefs.putUShort("magic", CRED_MAGIC_ENCRYPTED);
  prefs.putBytes("essid", ssidBlob, WIFI_ENC_SSID_BLOB);
  prefs.putBytes("epass", passBlob, WIFI_ENC_PASS_BLOB);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
}

static inline void clearWifiConfig() {
  saveWifiConfig("", "");
}

#endif
