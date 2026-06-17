#pragma once

#include <Arduino.h>
#include "config.h"
#include "credential_crypto.h"

#ifndef PAGE_TITLE_DEFAULT
#define PAGE_TITLE_DEFAULT "Tribe Repeater Node"
#endif

static constexpr uint8_t AP_SSID_MAX = 32;
static constexpr uint8_t AP_PASS_MAX = 64;
static constexpr uint8_t AP_TITLE_MAX = 48;
static constexpr size_t AP_ENC_SSID_BLOB = 1 + CRED_NONCE_LEN + AP_SSID_MAX;
static constexpr size_t AP_ENC_PASS_BLOB = 1 + CRED_NONCE_LEN + AP_PASS_MAX;
static constexpr size_t AP_ENC_TITLE_BLOB = 1 + CRED_NONCE_LEN + AP_TITLE_MAX;

static uint8_t gApFlags = AP_FLAG_PRIVACY;

static inline uint8_t apStoreGetFlags() {
  return gApFlags;
}

static inline bool apStorePrivacyRedact() {
  return (gApFlags & AP_FLAG_PRIVACY) != 0;
}

static inline bool apStoreApHidden() {
  return (gApFlags & AP_FLAG_HIDDEN) != 0;
}

static inline void apStoreSetFlags(uint8_t flags) {
  gApFlags = flags;
}

#if defined(ESP8266)

#include <EEPROM.h>

static constexpr uint16_t AP_EEPROM_BASE = 200;

static inline void apReadLegacyPlain(String &apSsid, String &apPass, String &pageTitle) {
  char buf[AP_PASS_MAX + 1] = {0};
  for (uint8_t i = 0; i < AP_SSID_MAX; i++) {
    buf[i] = static_cast<char>(EEPROM.read(AP_EEPROM_BASE + 2 + i));
  }
  if (buf[0] != '\0') {
    apSsid = String(buf);
  }
  for (uint8_t i = 0; i < AP_PASS_MAX; i++) {
    buf[i] = static_cast<char>(EEPROM.read(AP_EEPROM_BASE + 34 + i));
  }
  apPass = String(buf);
  for (uint8_t i = 0; i < AP_TITLE_MAX; i++) {
    buf[i] = static_cast<char>(EEPROM.read(AP_EEPROM_BASE + 98 + i));
  }
  if (buf[0] != '\0') {
    pageTitle = String(buf);
  }
}

static inline void loadApSettings(String &apSsid, String &apPass, String &pageTitle) {
  apSsid = AP_SSID;
  apPass = AP_PASS;
  pageTitle = PAGE_TITLE_DEFAULT;
  gApFlags = AP_FLAG_PRIVACY;

  EEPROM.begin(512);
  const uint16_t magic = EEPROM.read(AP_EEPROM_BASE) |
                         (static_cast<uint16_t>(EEPROM.read(AP_EEPROM_BASE + 1)) << 8);
  if (magic == CRED_MAGIC_AP_LEGACY) {
    apReadLegacyPlain(apSsid, apPass, pageTitle);
    EEPROM.end();
    return;
  }
  if (magic != CRED_MAGIC_ENCRYPTED) {
    EEPROM.end();
    return;
  }
  gApFlags = EEPROM.read(AP_EEPROM_BASE + 2);
  uint16_t off = AP_EEPROM_BASE + 3;
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t blob[AP_ENC_SSID_BLOB];
  for (size_t i = 0; i < AP_ENC_SSID_BLOB; i++) {
    blob[i] = EEPROM.read(off + i);
  }
  char buf[AP_SSID_MAX + 1];
  if (credDecryptField(blob, AP_ENC_SSID_BLOB, AP_SSID_MAX, buf, sizeof(buf), key)) {
    apSsid = String(buf);
  }
  off += AP_ENC_SSID_BLOB;
  for (size_t i = 0; i < AP_ENC_PASS_BLOB; i++) {
    blob[i] = EEPROM.read(off + i);
  }
  if (credDecryptField(blob, AP_ENC_PASS_BLOB, AP_PASS_MAX, buf, sizeof(buf), key)) {
    apPass = String(buf);
  }
  off += AP_ENC_PASS_BLOB;
  for (size_t i = 0; i < AP_ENC_TITLE_BLOB; i++) {
    blob[i] = EEPROM.read(off + i);
  }
  if (credDecryptField(blob, AP_ENC_TITLE_BLOB, AP_TITLE_MAX, buf, sizeof(buf), key)) {
    pageTitle = String(buf);
  }
  EEPROM.end();
}

static inline void saveApSettings(const String &apSsid, const String &apPass,
                                  const String &pageTitle) {
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t ssidBlob[AP_ENC_SSID_BLOB];
  uint8_t passBlob[AP_ENC_PASS_BLOB];
  uint8_t titleBlob[AP_ENC_TITLE_BLOB];
  credEncryptField(apSsid.c_str(), AP_SSID_MAX, ssidBlob, sizeof(ssidBlob), key);
  credEncryptField(apPass.c_str(), AP_PASS_MAX, passBlob, sizeof(passBlob), key);
  credEncryptField(pageTitle.c_str(), AP_TITLE_MAX, titleBlob, sizeof(titleBlob), key);

  EEPROM.begin(512);
  EEPROM.write(AP_EEPROM_BASE, CRED_MAGIC_ENCRYPTED & 0xFF);
  EEPROM.write(AP_EEPROM_BASE + 1, (CRED_MAGIC_ENCRYPTED >> 8) & 0xFF);
  EEPROM.write(AP_EEPROM_BASE + 2, gApFlags);
  uint16_t off = AP_EEPROM_BASE + 3;
  for (size_t i = 0; i < AP_ENC_SSID_BLOB; i++) {
    EEPROM.write(off + i, ssidBlob[i]);
  }
  off += AP_ENC_SSID_BLOB;
  for (size_t i = 0; i < AP_ENC_PASS_BLOB; i++) {
    EEPROM.write(off + i, passBlob[i]);
  }
  off += AP_ENC_PASS_BLOB;
  for (size_t i = 0; i < AP_ENC_TITLE_BLOB; i++) {
    EEPROM.write(off + i, titleBlob[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

#else

#include <Preferences.h>

static inline void loadApSettings(String &apSsid, String &apPass, String &pageTitle) {
  apSsid = AP_SSID;
  apPass = AP_PASS;
  pageTitle = PAGE_TITLE_DEFAULT;
  gApFlags = AP_FLAG_PRIVACY;

  Preferences prefs;
  if (!prefs.begin("repeaterui", true)) {
    return;
  }
  const uint16_t magic = static_cast<uint16_t>(prefs.getUShort("magic", 0));
  if (magic == CRED_MAGIC_AP_LEGACY || magic == 0) {
    apSsid = prefs.getString("ap_ssid", apSsid);
    apPass = prefs.getString("ap_pass", apPass);
    pageTitle = prefs.getString("title", pageTitle);
    gApFlags = static_cast<uint8_t>(prefs.getUChar("flags", AP_FLAG_PRIVACY));
    prefs.end();
    return;
  }
  if (magic != CRED_MAGIC_ENCRYPTED) {
    prefs.end();
    return;
  }
  gApFlags = prefs.getUChar("flags", AP_FLAG_PRIVACY);
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  size_t len = prefs.getBytesLength("essid");
  if (len == AP_ENC_SSID_BLOB) {
    uint8_t blob[AP_ENC_SSID_BLOB];
    prefs.getBytes("essid", blob, len);
    char buf[AP_SSID_MAX + 1];
    if (credDecryptField(blob, len, AP_SSID_MAX, buf, sizeof(buf), key)) {
      apSsid = String(buf);
    }
  }
  len = prefs.getBytesLength("epass");
  if (len == AP_ENC_PASS_BLOB) {
    uint8_t blob[AP_ENC_PASS_BLOB];
    prefs.getBytes("epass", blob, len);
    char buf[AP_PASS_MAX + 1];
    if (credDecryptField(blob, len, AP_PASS_MAX, buf, sizeof(buf), key)) {
      apPass = String(buf);
    }
  }
  len = prefs.getBytesLength("etitle");
  if (len == AP_ENC_TITLE_BLOB) {
    uint8_t blob[AP_ENC_TITLE_BLOB];
    prefs.getBytes("etitle", blob, len);
    char buf[AP_TITLE_MAX + 1];
    if (credDecryptField(blob, len, AP_TITLE_MAX, buf, sizeof(buf), key)) {
      pageTitle = String(buf);
    }
  }
  prefs.end();
}

static inline void saveApSettings(const String &apSsid, const String &apPass,
                                  const String &pageTitle) {
  uint8_t key[CRED_KEY_LEN];
  credDeriveKey(key);
  uint8_t ssidBlob[AP_ENC_SSID_BLOB];
  uint8_t passBlob[AP_ENC_PASS_BLOB];
  uint8_t titleBlob[AP_ENC_TITLE_BLOB];
  credEncryptField(apSsid.c_str(), AP_SSID_MAX, ssidBlob, sizeof(ssidBlob), key);
  credEncryptField(apPass.c_str(), AP_PASS_MAX, passBlob, sizeof(passBlob), key);
  credEncryptField(pageTitle.c_str(), AP_TITLE_MAX, titleBlob, sizeof(titleBlob), key);

  Preferences prefs;
  prefs.begin("repeaterui", false);
  prefs.putUShort("magic", CRED_MAGIC_ENCRYPTED);
  prefs.putUChar("flags", gApFlags);
  prefs.putBytes("essid", ssidBlob, AP_ENC_SSID_BLOB);
  prefs.putBytes("epass", passBlob, AP_ENC_PASS_BLOB);
  prefs.putBytes("etitle", titleBlob, AP_ENC_TITLE_BLOB);
  prefs.remove("ap_ssid");
  prefs.remove("ap_pass");
  prefs.remove("title");
  prefs.end();
}

#endif
