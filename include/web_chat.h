#pragma once

#include <Arduino.h>
#include "web_ui.h"

#if defined(ESP8266)
#include <ESP8266WebServer.h>
using ChatServer = ESP8266WebServer;
#else
#include <WebServer.h>
using ChatServer = WebServer;
#endif

static constexpr uint8_t CHAT_MAX_MSGS = 24;
static constexpr uint8_t CHAT_TEXT_LEN = 96;
static constexpr uint8_t CHAT_MAX_USERS = 12;
static constexpr uint32_t CHAT_USER_TTL_MS = 120000;

struct ChatMessage {
  uint32_t id = 0;
  uint32_t userHash = 0;
  uint32_t ts = 0;
  bool isSystem = false;
  char text[CHAT_TEXT_LEN] = {0};
};

struct ChatUser {
  uint32_t hash = 0;
  uint32_t lastSeenMs = 0;
  bool announced = false;
};

static ChatMessage chatRing[CHAT_MAX_MSGS];
static uint8_t chatHead = 0;
static uint8_t chatCount = 0;
static uint32_t chatNextId = 1;
static ChatUser chatUsers[CHAT_MAX_USERS];

static inline uint32_t chatHashUserAgent(const String &ua) {
  uint32_t h = 2166136261u;
  for (unsigned i = 0; i < ua.length(); i++) {
    h ^= static_cast<uint8_t>(ua[i]);
    h *= 16777619u;
  }
  return h;
}

static inline String chatNickFromHash(uint32_t h) {
  char buf[12];
  snprintf(buf, sizeof(buf), "#%08lx", static_cast<unsigned long>(h));
  return String(buf);
}

static inline String chatJsonEscape(const String &s) {
  String o;
  o.reserve(s.length() + 8);
  for (unsigned i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '\\' || c == '"') {
      o += '\\';
      o += c;
    } else if (c == '\n' || c == '\r') {
      o += ' ';
    } else {
      o += c;
    }
  }
  return o;
}

static inline String chatHtmlEscape(const String &s) {
  String o;
  o.reserve(s.length() + 16);
  for (unsigned i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '<') {
      o += "&lt;";
    } else if (c == '>') {
      o += "&gt;";
    } else if (c == '&') {
      o += "&amp;";
    } else if (c == '"') {
      o += "&quot;";
    } else {
      o += c;
    }
  }
  return o;
}

static inline void chatTouchUser(uint32_t hash, uint32_t nowMs) {
  for (uint8_t i = 0; i < CHAT_MAX_USERS; i++) {
    if (chatUsers[i].hash == hash) {
      chatUsers[i].lastSeenMs = nowMs;
      return;
    }
  }
  for (uint8_t i = 0; i < CHAT_MAX_USERS; i++) {
    if (chatUsers[i].hash == 0) {
      chatUsers[i].hash = hash;
      chatUsers[i].lastSeenMs = nowMs;
      chatUsers[i].announced = false;
      return;
    }
  }
  chatUsers[0] = {hash, nowMs, false};
}

static inline void chatPush(uint32_t userHash, const String &text, bool isSystem) {
  ChatMessage &m = chatRing[chatHead];
  m.id = chatNextId++;
  m.userHash = userHash;
  m.ts = millis() / 1000;
  m.isSystem = isSystem;
  text.substring(0, CHAT_TEXT_LEN - 1).toCharArray(m.text, CHAT_TEXT_LEN);
  chatHead = (chatHead + 1) % CHAT_MAX_MSGS;
  if (chatCount < CHAT_MAX_MSGS) {
    chatCount++;
  }
}

static inline void chatMaybeAnnounceJoin(uint32_t hash) {
  for (uint8_t i = 0; i < CHAT_MAX_USERS; i++) {
    if (chatUsers[i].hash == hash && !chatUsers[i].announced) {
      chatUsers[i].announced = true;
      chatPush(0, chatNickFromHash(hash) + " joined the channel", true);
      return;
    }
  }
}

static inline void chatInit() {
  memset(chatRing, 0, sizeof(chatRing));
  memset(chatUsers, 0, sizeof(chatUsers));
  chatHead = 0;
  chatCount = 0;
  chatNextId = 1;
  chatPush(0, "node chat online — messages die on power-off (no cloud)", true);
}

static inline String chatGetUserAgent(ChatServer &server) {
  if (server.hasHeader("User-Agent")) {
    return server.header("User-Agent");
  }
  return "unknown";
}

static inline void chatHandlePoll(ChatServer &server) {
  const uint32_t since = server.hasArg("since") ? server.arg("since").toInt() : 0;
  const uint32_t nowMs = millis();
  const uint32_t pollHash = chatHashUserAgent(chatGetUserAgent(server));
  if (pollHash != 0) {
    chatTouchUser(pollHash, nowMs);
  }
  String json = "{\"messages\":[";
  bool first = true;
  for (uint8_t i = 0; i < chatCount; i++) {
    const uint8_t idx =
        (chatHead + CHAT_MAX_MSGS - chatCount + i) % CHAT_MAX_MSGS;
    const ChatMessage &m = chatRing[idx];
    if (m.id <= since) {
      continue;
    }
    if (!first) {
      json += ',';
    }
    first = false;
    json += "{\"id\":" + String(m.id) + ",\"ts\":" + String(m.ts) +
            ",\"sys\":" + String(m.isSystem ? 1 : 0) +
            ",\"user\":\"" + chatJsonEscape(chatNickFromHash(m.userHash)) +
            "\",\"text\":\"" + chatJsonEscape(String(m.text)) + "\"}";
  }
  json += "],\"users\":[";
  first = true;
  for (uint8_t i = 0; i < CHAT_MAX_USERS; i++) {
    if (chatUsers[i].hash == 0 ||
        nowMs - chatUsers[i].lastSeenMs > CHAT_USER_TTL_MS) {
      continue;
    }
    if (!first) {
      json += ',';
    }
    first = false;
    json += "{\"nick\":\"" + chatJsonEscape(chatNickFromHash(chatUsers[i].hash)) +
            "\"}";
  }
  json += "],\"max\":" + String(CHAT_MAX_MSGS) + "}";
  server.send(200, "application/json", json);
}

static inline void chatHandleSend(ChatServer &server) {
  if (!server.hasArg("msg")) {
    server.send(400, "text/plain", "missing msg");
    return;
  }
  String msg = server.arg("msg");
  msg.trim();
  if (msg.length() == 0) {
    server.send(400, "text/plain", "empty");
    return;
  }
  if (msg.length() > CHAT_TEXT_LEN - 1) {
    msg = msg.substring(0, CHAT_TEXT_LEN - 1);
  }
  const String ua = chatGetUserAgent(server);
  const uint32_t hash = chatHashUserAgent(ua);
  const uint32_t nowMs = millis();
  chatTouchUser(hash, nowMs);
  chatMaybeAnnounceJoin(hash);
  chatPush(hash, msg, false);
  server.send(200, "application/json",
               "{\"ok\":1,\"id\":" + String(chatNextId - 1) + "}");
}

static inline String chatPageBody(const char *apSsid) {
  String body;
  body.reserve(1200);
  body += F("<p class=\"chat-warn\"><b>Single-node IRC.</b> This room exists only on "
            "this repeater. Other nodes have separate logs. Power off = total amnesia.</p>");
  body += F("<p class=\"chat-meta\">Channel <code>");
  body += chatHtmlEscape(String(apSsid));
  body += F("</code> · You are identified by a hash of your browser User-Agent.</p>");
  body += F("<div id=\"chatLog\" class=\"chat-log\" aria-live=\"polite\"></div>");
  body += F("<form id=\"chatForm\" class=\"chat-form\">"
            "<label for=\"chatIn\">Message</label>"
            "<input id=\"chatIn\" maxlength=\"95\" autocomplete=\"off\" placeholder=\"say something…\">"
            "<button type=\"submit\">Send</button></form>");
  body += FPSTR(WEB_UI_CHAT_SCRIPT);
  return body;
}

static inline void chatHandlePage(ChatServer &server, const char *apSsid,
                                  const char *pageTitle) {
  server.send(200, "text/html", htmlPage(chatPageBody(apSsid), apSsid, pageTitle));
}
