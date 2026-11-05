"""Минимальная обёртка над Telegram Bot API (через requests)."""
import requests

from . import config

BASE = "https://api.telegram.org"


class TelegramError(RuntimeError):
    def __init__(self, method, code):
        self.code = code
        super().__init__(f"Telegram {method}: {code}")


def call(method, **params):
    r = requests.post(f"{BASE}/bot{config.BOT_TOKEN}/{method}", json=params, timeout=40)
    data = r.json()
    if not data.get("ok"):
        raise TelegramError(method, r.status_code)
    return data["result"]


def send(chat_id, text, buttons=None):
    params = {"chat_id": chat_id, "text": text}
    if buttons:
        params["reply_markup"] = {"inline_keyboard": buttons}
    return call("sendMessage", **params)


def answer_callback(callback_id):
    try:
        call("answerCallbackQuery", callback_query_id=callback_id)
    except Exception:  # noqa: BLE001
        pass


def updates(offset):
    return call("getUpdates", offset=offset, timeout=0, allowed_updates=["message", "callback_query"])


def download_photo(file_id, dest):
    info = call("getFile", file_id=file_id)
    with requests.get(f"{BASE}/file/bot{config.BOT_TOKEN}/{info['file_path']}",
                      timeout=40, stream=True) as response:
        response.raise_for_status()
        with open(dest, "wb") as file:
            size = 0
            for chunk in response.iter_content(65536):
                size += len(chunk)
                if size > 20 * 1024 * 1024:
                    raise ValueError("Receipt image is too large")
                file.write(chunk)
