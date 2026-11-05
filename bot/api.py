"""Клиент сервера Car Log для бота."""
import requests

from . import config


class ServerError(Exception):
    def __init__(self, message, code="error", status=0):
        super().__init__(message)
        self.message, self.code, self.status = message, code, status


def _call(method, path, **kwargs):
    try:
        r = requests.request(method, config.SERVER_URL + path, headers={"X-Bot-Secret": config.BOT_SECRET},
                             timeout=15, **kwargs)
    except requests.RequestException:
        raise ServerError("Сервер Car Log недоступен. Попробуйте позже.", "offline")
    try:
        data = r.json()
    except ValueError:
        raise ServerError("Сервер вернул неожиданный ответ.", "bad_response", r.status_code)
    if r.status_code >= 400:
        raise ServerError(data.get("error", "Ошибка сервера."), data.get("code", "error"), r.status_code)
    return data


def link(chat_id, code):
    return _call("POST", "/api/bot/link", json={"chat_id": chat_id, "code": code})


def unlink(chat_id):
    return _call("POST", "/api/bot/unlink", json={"chat_id": chat_id})


def cars(chat_id):
    return _call("GET", "/api/bot/cars", params={"chat_id": chat_id})


def add_fuel(chat_id, **fields):
    return _call("POST", "/api/bot/fuel", json={"chat_id": chat_id, **fields})


def due_reminders():
    return _call("GET", "/api/bot/reminders/due")


def mark_notified(item):
    fields = {key: item[key] for key in ("chat_id", "version", "stage")}
    return _call("POST", f"/api/bot/reminders/{item['id']}/notified", json=fields)


def upcoming(chat_id):
    return _call("GET", "/api/bot/reminders/upcoming", params={"chat_id": chat_id})


def outbox():
    return _call("GET", "/api/bot/outbox")


def outbox_sent(msg_id):
    return _call("POST", f"/api/bot/outbox/{msg_id}/sent")


def complete_reminder(chat_id, record_id):
    return _call("POST", f"/api/bot/reminders/{record_id}/done", json={"chat_id": chat_id})
