#pragma once

#include <Arduino.h>

#ifndef CREDENTIAL_PEPPER
#define CREDENTIAL_PEPPER "tw-change-me"
#endif

static constexpr uint16_t CRED_MAGIC_ENCRYPTED = 0xE1C0;
static constexpr uint16_t CRED_MAGIC_WIFI_LEGACY = 0xA55A;
static constexpr uint16_t CRED_MAGIC_AP_LEGACY = 0xC0DE;

static constexpr uint8_t CRED_NONCE_LEN = 8;
static constexpr uint8_t CRED_KEY_LEN = 16;

// AP / node flags (ap_store byte 2 when encrypted)
static constexpr uint8_t AP_FLAG_HIDDEN = 0x01;
static constexpr uint8_t AP_FLAG_PRIVACY = 0x02;

#if defined(ESP8266)
extern "C" {
#include "bearssl/bearssl_block.h"
#include "bearssl/bearssl_hash.h"
}
#else
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#endif

static inline uint64_t credChipId() {
#if defined(ESP8266)
  return ESP.getChipId();
#else
  uint64_t mac = ESP.getEfuseMac();
  return mac ^ (mac >> 32);
#endif
}

static inline void credDeriveKey(uint8_t key[CRED_KEY_LEN]) {
  uint8_t buf[32];
  const uint64_t chip = credChipId();
  memcpy(buf, &chip, sizeof(chip));
  size_t pepperLen = strlen(CREDENTIAL_PEPPER);
  if (pepperLen > 16) {
    pepperLen = 16;
  }
  memcpy(buf + sizeof(chip), CREDENTIAL_PEPPER, pepperLen);
  const size_t totalLen = sizeof(chip) + pepperLen;

#if defined(ESP8266)
  uint8_t hash[32];
  br_sha256_context ctx;
  br_sha256_init(&ctx);
  br_sha256_update(&ctx, buf, totalLen);
  br_sha256_out(&ctx, hash);
#else
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, buf, totalLen);
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);
#endif
  memcpy(key, hash, CRED_KEY_LEN);
}

#if defined(ESP8266)
static inline void credAesCtrCrypt(const uint8_t key[CRED_KEY_LEN], const uint8_t nonce[CRED_NONCE_LEN],
                                   uint8_t *data, size_t len) {
  br_aes_small_ctr_keys ctr;
  br_aes_small_ctr_init(&ctr, key, CRED_KEY_LEN);
  uint8_t iv[16] = {0};
  memcpy(iv, nonce, CRED_NONCE_LEN);
  br_aes_small_ctr_run(&ctr, iv, 0, data, len);
}
#else
static inline void credAesCtrCrypt(const uint8_t key[CRED_KEY_LEN], const uint8_t nonce[CRED_NONCE_LEN],
                                   uint8_t *data, size_t len) {
  uint8_t iv[16] = {0};
  memcpy(iv, nonce, CRED_NONCE_LEN);
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 128);
  size_t nc_off = 0;
  uint8_t stream_block[16];
  mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv, stream_block, data, data);
  mbedtls_aes_free(&aes);
}
#endif

static inline size_t credEncryptField(const char *plain, uint8_t maxPlain, uint8_t *out,
                                      size_t outCap, const uint8_t key[CRED_KEY_LEN]) {
  if (outCap < CRED_NONCE_LEN + maxPlain + 1) {
    return 0;
  }
  uint8_t len = 0;
  while (len < maxPlain && plain[len] != '\0') {
    len++;
  }
  out[0] = len;
  for (uint8_t i = 0; i < CRED_NONCE_LEN; i++) {
#if defined(ESP8266)
    out[1 + i] = static_cast<uint8_t>(random(256));
#else
    out[1 + i] = static_cast<uint8_t>(esp_random() & 0xFF);
#endif
  }
  memcpy(out + 1 + CRED_NONCE_LEN, plain, len);
  if (len < maxPlain) {
    memset(out + 1 + CRED_NONCE_LEN + len, 0, maxPlain - len);
  }
  credAesCtrCrypt(key, out + 1, out + 1 + CRED_NONCE_LEN, maxPlain);
  return 1 + CRED_NONCE_LEN + maxPlain;
}

static inline bool credDecryptField(const uint8_t *in, size_t inLen, uint8_t maxPlain, char *out,
                                    size_t outCap, const uint8_t key[CRED_KEY_LEN]) {
  if (inLen < 1 + CRED_NONCE_LEN + maxPlain || outCap < maxPlain + 1) {
    return false;
  }
  const uint8_t len = in[0];
  if (len > maxPlain) {
    return false;
  }
  uint8_t buf[64];
  if (maxPlain > sizeof(buf)) {
    return false;
  }
  memcpy(buf, in + 1 + CRED_NONCE_LEN, maxPlain);
  credAesCtrCrypt(key, in + 1, buf, maxPlain);
  memcpy(out, buf, len);
  out[len] = '\0';
  return true;
}

// Reject control chars / ANSI garbage from corrupted serial input or flash
static inline bool credStringIsValid(const String &s) {
  if (s.length() == 0) {
    return true;
  }
  for (unsigned i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c < 0x20 || c > 0x7E) {
      return false;
    }
  }
  return true;
}

static inline String credSanitizeString(const String &s) {
  if (credStringIsValid(s)) {
    return s;
  }
  String o;
  o.reserve(s.length());
  for (unsigned i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c >= 0x20 && c <= 0x7E) {
      o += c;
    }
  }
  return o;
}
