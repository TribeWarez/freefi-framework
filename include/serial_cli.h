#pragma once

#include <Arduino.h>
#include "ap_store.h"
#include "wifi_store.h"
#include "wifi_platform.h"
#include "credential_crypto.h"

struct SerialCliContext {
  String *uplinkSsid = nullptr;
  String *uplinkPass = nullptr;
  String *apSsid = nullptr;
  String *apPass = nullptr;
  String *pageTitle = nullptr;
  WifiPlatformState *wifiState = nullptr;
  void (*onSaveUplink)(const String &ssid, const String &pass) = nullptr;
  void (*onClearUplink)() = nullptr;
  void (*onSaveAp)() = nullptr;
  void (*onRestartAp)() = nullptr;
  void (*onConnectUplink)() = nullptr;
  void (*onRefreshOled)() = nullptr;
};

static String gSerialLine;
static uint8_t gEscState = 0;

static inline void serialCliStripAnsi(String &line) {
  String out;
  out.reserve(line.length());
  for (unsigned i = 0; i < line.length(); i++) {
    const char c = line[i];
    if (c == '\x1b') {
      while (i + 1 < line.length() && line[i + 1] != ' ' && (line[i + 1] < 0x40 || line[i + 1] > 0x7E)) {
        i++;
      }
      continue;
    }
    if (c >= 0x20 && c <= 0x7E) {
      out += c;
    }
  }
  line = out;
}

static inline bool serialCliParseQuoted(const String &rest, String &a, String &b) {
  a = "";
  b = "";
  unsigned i = 0;
  while (i < rest.length() && rest[i] == ' ') {
    i++;
  }
  if (i >= rest.length()) {
    return false;
  }
  if (rest[i] == '"') {
    i++;
    while (i < rest.length() && rest[i] != '"') {
      a += rest[i++];
    }
    if (i < rest.length() && rest[i] == '"') {
      i++;
    }
    while (i < rest.length() && rest[i] == ' ') {
      i++;
    }
    if (i < rest.length() && rest[i] == '"') {
      i++;
      while (i < rest.length() && rest[i] != '"') {
        b += rest[i++];
      }
      return a.length() > 0;
    }
    b = rest.substring(i);
    b.trim();
    return a.length() > 0;
  }
  const int sp = rest.indexOf(' ', i);
  if (sp < 0) {
    a = rest.substring(i);
    a.trim();
    return a.length() > 0;
  }
  a = rest.substring(i, sp);
  b = rest.substring(sp + 1);
  b.trim();
  return a.length() > 0;
}

static inline void serialCliPrintHelp() {
  Serial.println(F("Tribe node serial admin (115200)"));
  Serial.println(F("  help"));
  Serial.println(F("  status"));
  Serial.println(F("  show"));
  Serial.println(F("  uplink <ssid> <pass>"));
  Serial.println(F("  uplink \"ssid with spaces\" \"pass\""));
  Serial.println(F("  uplink clear"));
  Serial.println(F("  ap <ssid> [pass] [hidden]"));
  Serial.println(F("  title <name>"));
  Serial.println(F("  privacy on|off"));
  Serial.println(F("  reboot"));
  Serial.println(F("Tip: do not use arrow keys in the monitor — they corrupt input."));
}

static inline void serialCliCmdStatus(const SerialCliContext &ctx) {
  Serial.print(F("AP clients: "));
  Serial.println(WiFi.softAPgetStationNum());
  Serial.print(F("NAT: "));
  Serial.println(ctx.wifiState != nullptr && ctx.wifiState->naptEnabled ? F("on") : F("off"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Uplink: connected IP "));
    Serial.println(WiFi.localIP());
    Serial.print(F("RSSI: "));
    Serial.println(WiFi.RSSI());
  } else if (ctx.uplinkSsid != nullptr && ctx.uplinkSsid->length() > 0) {
    Serial.println(F("Uplink: connecting (or wrong SSID/pass)"));
  } else {
    Serial.println(F("Uplink: not configured"));
  }
  Serial.print(F("Privacy redact: "));
  Serial.println(apStorePrivacyRedact() ? F("on") : F("off"));
  Serial.print(F("AP hidden: "));
  Serial.println(apStoreApHidden() ? F("yes") : F("no"));
}

static inline void serialCliCmdShow(const SerialCliContext &ctx) {
  if (ctx.apSsid != nullptr) {
    Serial.print(F("AP SSID: "));
    Serial.println(*ctx.apSsid);
  }
  if (ctx.uplinkSsid != nullptr) {
    Serial.print(F("Uplink SSID: "));
    if (ctx.uplinkSsid->length() == 0) {
      Serial.println(F("(none)"));
    } else if (!credStringIsValid(*ctx.uplinkSsid)) {
      Serial.println(F("(invalid — run: uplink clear)"));
    } else {
      Serial.println(*ctx.uplinkSsid);
    }
  }
}

static inline void serialCliDispatch(const SerialCliContext &ctx, String line) {
  serialCliStripAnsi(line);
  line.trim();
  if (line.length() == 0) {
    return;
  }
  if (line == F("help") || line == F("?")) {
    serialCliPrintHelp();
    return;
  }
  if (line == F("status")) {
    serialCliCmdStatus(ctx);
    return;
  }
  if (line == F("show")) {
    serialCliCmdShow(ctx);
    return;
  }
  if (line == F("reboot")) {
    Serial.println(F("Rebooting..."));
    delay(100);
    ESP.restart();
  }
  if (line.startsWith(F("privacy "))) {
    String arg = line.substring(8);
    arg.trim();
    if (arg == F("on")) {
      apStoreSetFlags(apStoreGetFlags() | AP_FLAG_PRIVACY);
    } else if (arg == F("off")) {
      apStoreSetFlags(apStoreGetFlags() & ~static_cast<uint8_t>(AP_FLAG_PRIVACY));
    } else {
      Serial.println(F("Use: privacy on|off"));
      return;
    }
    if (ctx.apSsid != nullptr) {
      saveApSettings(*ctx.apSsid, *ctx.apPass, *ctx.pageTitle);
      if (ctx.onSaveAp != nullptr) {
        ctx.onSaveAp();
      }
    }
    Serial.println(F("OK"));
    return;
  }
  if (line.startsWith(F("title "))) {
    const String t = line.substring(6);
    if (t.length() == 0 || t.length() > AP_TITLE_MAX) {
      Serial.println(F("Invalid title"));
      return;
    }
    if (ctx.pageTitle != nullptr) {
      *ctx.pageTitle = t;
      saveApSettings(*ctx.apSsid, *ctx.apPass, *ctx.pageTitle);
      if (ctx.onRefreshOled != nullptr) {
        ctx.onRefreshOled();
      }
    }
    Serial.println(F("OK"));
    return;
  }
  if (line.startsWith(F("uplink "))) {
    const String rest = line.substring(7);
    if (rest.equalsIgnoreCase(F("clear"))) {
      if (ctx.onClearUplink != nullptr) {
        ctx.onClearUplink();
      }
      Serial.println(F("Uplink cleared"));
      return;
    }
    String ssid;
    String pass;
    if (!serialCliParseQuoted(rest, ssid, pass)) {
      Serial.println(F("Use: uplink <ssid> <pass>  or  uplink clear"));
      return;
    }
    ssid = credSanitizeString(ssid);
    pass = credSanitizeString(pass);
    if (ssid.length() == 0 || ssid.length() > WIFI_SSID_MAX) {
      Serial.println(F("Invalid SSID"));
      return;
    }
    if (ctx.onSaveUplink != nullptr) {
      ctx.onSaveUplink(ssid, pass);
    }
    Serial.println(F("OK — connecting"));
    return;
  }
  if (line.startsWith(F("ap "))) {
    String rest = line.substring(3);
    rest.trim();
    serialCliStripAnsi(rest);
    int sp1 = rest.indexOf(' ');
    String ssid;
    String pass;
    bool hidden = false;
    if (sp1 < 0) {
      ssid = rest;
    } else {
      ssid = rest.substring(0, sp1);
      rest = rest.substring(sp1 + 1);
      rest.trim();
      if (rest.equalsIgnoreCase(F("hidden"))) {
        hidden = true;
      } else {
        int sp2 = rest.indexOf(' ');
        if (sp2 < 0) {
          pass = rest;
        } else {
          pass = rest.substring(0, sp2);
          rest = rest.substring(sp2 + 1);
          rest.trim();
          if (rest.equalsIgnoreCase(F("hidden"))) {
            hidden = true;
          }
        }
      }
    }
    ssid = credSanitizeString(ssid);
    if (ssid.length() == 0 || ssid.length() > AP_SSID_MAX) {
      Serial.println(F("Invalid AP SSID"));
      return;
    }
    if (ctx.apSsid != nullptr) {
      *ctx.apSsid = ssid;
      *ctx.apPass = credSanitizeString(pass);
      uint8_t flags = apStoreGetFlags();
      if (hidden) {
        flags |= AP_FLAG_HIDDEN;
      } else {
        flags &= ~static_cast<uint8_t>(AP_FLAG_HIDDEN);
      }
      apStoreSetFlags(flags);
      saveApSettings(*ctx.apSsid, *ctx.apPass, *ctx.pageTitle);
      if (ctx.onRestartAp != nullptr) {
        ctx.onRestartAp();
      }
    }
    Serial.println(F("OK"));
    return;
  }
  Serial.println(F("Unknown command — type help"));
}

static inline void serialCliFeedChar(const SerialCliContext &ctx, char c) {
  if (c == '\r') {
    return;
  }
  if (c == '\n') {
    serialCliDispatch(ctx, gSerialLine);
    gSerialLine = "";
    gEscState = 0;
    return;
  }
  if (c == '\x1b') {
    gEscState = 1;
    return;
  }
  if (gEscState == 1) {
    gEscState = (c == '[') ? 2 : 0;
    return;
  }
  if (gEscState == 2) {
    if ((c >= '0' && c <= '9') || c == ';') {
      return;
    }
    gEscState = 0;
    return;
  }
  if (c == '\x7f' || c == '\x08') {
    if (gSerialLine.length() > 0) {
      gSerialLine.remove(gSerialLine.length() - 1);
    }
    return;
  }
  if (c >= 0x20 && c < 0x7f && gSerialLine.length() < 120) {
    gSerialLine += c;
  }
}

static inline void serialCliPoll(const SerialCliContext &ctx) {
  while (Serial.available() > 0) {
    serialCliFeedChar(ctx, static_cast<char>(Serial.read()));
  }
}
