"""Ephemeral IRC-style chat (RAM only), compatible with ESP web UI."""
import hashlib
import time
from collections import deque

MAX_MSGS = 48
MAX_TEXT = 96
USER_TTL = 120


class ChatRoom:
    def __init__(self):
        self.messages = deque(maxlen=MAX_MSGS)
        self.next_id = 1
        self.users = {}
        self.push_system("node chat online — messages die on restart (no cloud)")

    def push_system(self, text):
        self._push(0, text, True)

    def _push(self, user_hash, text, is_system=False):
        mid = self.next_id
        self.next_id += 1
        self.messages.append(
            {
                "id": mid,
                "ts": int(time.time()),
                "sys": is_system,
                "user": nick_from_hash(user_hash),
                "text": text[:MAX_TEXT],
            }
        )
        return mid

    def touch_user(self, ua: str):
        h = hash_ua(ua)
        if not h:
            return h
        now = time.time()
        if h not in self.users:
            self.users[h] = {"announced": False, "last": now}
            self.push_system(f"{nick_from_hash(h)} joined")
            self.users[h]["announced"] = True
        else:
            self.users[h]["last"] = now
        return h

    def send(self, ua: str, msg: str):
        msg = (msg or "").strip()
        if not msg:
            return None
        h = self.touch_user(ua)
        return self._push(h, msg, False)

    def poll_json(self, since: int, ua: str):
        self.touch_user(ua)
        now = time.time()
        active = []
        for h, u in list(self.users.items()):
            if now - u["last"] > USER_TTL:
                del self.users[h]
            else:
                active.append({"nick": nick_from_hash(h)})
        msgs = [m for m in self.messages if m["id"] > since]
        return {"messages": msgs, "users": active, "max": MAX_MSGS}


def hash_ua(ua: str) -> int:
    if not ua:
        return 0
    d = hashlib.sha256(ua.encode()).digest()
    return int.from_bytes(d[:4], "big")


def nick_from_hash(h: int) -> str:
    if h == 0:
        return "system"
    return f"anon{h & 0xFFFF:04x}"
