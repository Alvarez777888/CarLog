"""Логика диалога бота. Не зависит от Telegram: принимает данные и возвращает ответы через функцию send."""
import re
import sys
from uuid import uuid4
from datetime import datetime

from . import api, receipt, menu

HELP = (
    "Я бот Car Log.\n\n"
    "Как начать:\n"
    "1. В программе Car Log получите код привязки.\n"
    "2. Отправьте мне: /link КОД\n\n"
    "Что я умею:\n"
    "• Пришлите фото чека АЗС, и я добавлю заправку.\n"
    "• /fuel ЛИТРЫ ЦЕНА [АЗС] — добавить заправку вручную.\n"
    "• /reminders — ближайшие напоминания.\n"
    "• /unlink — отвязать аккаунт."
)

pending = {}  # chat_id -> {"data": {...}, "stage": "...", "car": id}


def fmt_date(iso):
    return datetime.strptime(iso, "%Y-%m-%d").strftime("%d.%m.%Y")


def summary_text(d):
    def v(x, unit=""):
        return f"{x:g}{unit}" if x else "не найдено"
    lines = ["Распознал чек:",
             f"АЗС: {d['place'] or 'не найдено'}",
             f"Дата: {fmt_date(d['day'])}",
             f"Топливо: {d['fuel'] or '—'}",
             f"Литры: {v(d['liters'], ' л')}",
             f"Цена: {v(d['price'], ' ₽/л')}",
             f"Сумма: {v(d['amount'], ' ₽')}"]
    lines += [f"⚠ {w}" for w in d.get("warnings", [])]
    return "\n".join(lines)


CONFIRM_BUTTONS = [[{"text": "✅ Верно", "callback_data": "ok"}, {"text": "✏️ Исправить", "callback_data": "edit"}],
                   [{"text": "❌ Отмена", "callback_data": "cancel"}]]


def start_receipt(send, chat_id, text, source_key=None):
    data = receipt.parse_receipt(text)
    pending[chat_id] = {"data": data, "stage": "confirm", "source_key": source_key or uuid4().hex, "draft": uuid4().hex[:12]}
    if len(data["missing"]) == 3:
        pending.pop(chat_id, None)
        send(chat_id, "Не удалось распознать чек. Сфотографируйте ровнее и при хорошем свете "
                      "или нажмите «Добавить заправку».")
        return
    send(chat_id, summary_text(data), CONFIRM_BUTTONS)


def finish(send, chat_id, mileage):
    st = pending.get(chat_id)
    if not st:
        return
    d = st["data"]
    fields = dict(liters=d["liters"], price=d["price"], amount=d["amount"], place=d["place"], day=d["day"],
                  car=st["car"], source_key=st["source_key"], comment=("Чек: " + d["fuel"]) if d["fuel"] else "")
    if mileage:
        fields["mileage"] = mileage
    res = api.add_fuel(chat_id, **fields)
    pending.pop(chat_id, None)
    send(chat_id, f"✅ Заправка сохранена: {res['liters']:g} л × {res['price']:g} ₽ = {res['amount']:g} ₽"
                  f"{', ' + res['place'] if res['place'] else ''}.")


def telegram_buttons(chat_id, buttons):
    draft = pending.get(chat_id, {}).get("draft", "")
    return [[{**button, "callback_data": button["callback_data"] if button["callback_data"].startswith("menu:") else f"{draft}|{button['callback_data']}"} for button in row]
            for row in buttons]


def telegram_callback(send, chat_id, payload):
    if payload.startswith("menu:"):
        return menu.handle(send, chat_id, payload[5:], sys.modules[__name__])
    draft, _, action = payload.partition("|")
    state = pending.get(chat_id)
    if not state or state["draft"] != draft:
        return send(chat_id, "Эта кнопка относится к предыдущему чеку. Используйте кнопки под текущим чеком.")
    return on_callback(send, chat_id, action)


def on_callback(send, chat_id, data):
    st = pending.get(chat_id)
    if data == "cancel":
        pending.pop(chat_id, None)
        return send(chat_id, "Отменено.")
    if not st:
        return send(chat_id, "Эта операция уже неактуальна. Пришлите чек заново.")
    if data == "skip" and st["stage"] == "mileage":
        return finish(send, chat_id, 0)
    if data == "review":
        st["stage"] = "confirm"
        return send(chat_id, summary_text(st["data"]), CONFIRM_BUTTONS)
    if data.startswith("field:") and "data" in st:
        key = data[6:]
        if key in menu.LABELS:
            st.update(stage="field", field=key)
            return send(chat_id, "Введите " + menu.LABELS[key] + ".", [[menu.button("Назад", "review")]])
    if data == "edit" and st["stage"] == "confirm":
        st["stage"] = "edit"
        return send(chat_id, "Какое поле исправить?", menu.FIELDS)
    if data == "ok" and st["stage"] == "confirm":
        if st["data"]["missing"]:
            st["stage"] = "edit"
            return send(chat_id, "Не хватает данных (" + ", ".join(st["data"]["missing"]) +
                        "). Выберите поле для исправления.", menu.FIELDS)
        cars = api.cars(chat_id)
        if not cars:
            pending.pop(chat_id, None)
            return send(chat_id, "У вас нет автомобилей. Сначала добавьте автомобиль в программе.")
        if len(cars) == 1:
            st["car"], st["stage"] = cars[0]["id"], "mileage"
            return send(chat_id, "Введите текущий пробег или сохраните без него.", menu.MILEAGE)
        st["stage"] = "car"
        return send(chat_id, "Для какого автомобиля?",
                    [[{"text": f"{c['brand']} {c['model']}", "callback_data": f"car:{c['id']}"}] for c in cars])
    if data.startswith("car:") and st["stage"] == "car":
        st["car"], st["stage"] = int(data[4:]), "mileage"
        return send(chat_id, "Введите текущий пробег или сохраните без него.", menu.MILEAGE)


def on_text(send, chat_id, text):
    text = text.strip()
    if text.startswith("/start link_"):
        return on_text(send, chat_id, "/link " + text.split("link_",1)[1])
    if not text.startswith("/") and menu.text(send, chat_id, text, sys.modules[__name__]):
        return
    if text.startswith("/start") or text.startswith("/help"):
        return send(chat_id, "CarLog · Выберите действие.", menu.MENU)
    if text.startswith("/link"):
        code = text[5:].strip()
        if not code:
            return send(chat_id, "Отправьте код так: /link 123456")
        res = api.link(chat_id, code)
        pending.pop(chat_id, None)
        return send(chat_id, f"✅ Аккаунт «{res['login']}» привязан.")
    if text.startswith("/unlink"):
        api.unlink(chat_id)
        pending.pop(chat_id, None)
        return send(chat_id, "Аккаунт отвязан.")
    if text.startswith("/reminders"):
        items = api.upcoming(chat_id)
        if not items:
            return send(chat_id, "Напоминаний нет.")
        return send(chat_id, "Ближайшие напоминания:\n" + "\n".join(
            f"• {fmt_date(i['day'])} — {i['description']} ({i['brand']} {i['model']})" for i in items))
    if text.startswith("/fuel"):
        m = re.match(r"/fuel\s+(\d+[.,]?\d*)\s+(\d+[.,]?\d*)\s*(.*)$", text)
        if not m:
            return send(chat_id, "Формат: /fuel ЛИТРЫ ЦЕНА [АЗС], например: /fuel 42.5 58.9 Лукойл")
        liters, price = float(m.group(1).replace(",", ".")), float(m.group(2).replace(",", "."))
        pending[chat_id] = {"data": {"liters": liters, "price": price, "amount": round(liters * price, 2),
                                     "place": m.group(3).strip()[:60], "day": datetime.now().strftime("%Y-%m-%d"),
                                     "fuel": "", "missing": [], "warnings": []}, "stage": "confirm",
                            "source_key": uuid4().hex, "draft": uuid4().hex[:12]}
        return send(chat_id, summary_text(pending[chat_id]["data"]), CONFIRM_BUTTONS)

    st = pending.get(chat_id)
    if st and st["stage"] == "edit":
        try:
            st["data"].update(receipt.parse_edit(text))
        except ValueError as e:
            return send(chat_id, f"{e}")
        d = st["data"]
        if d.get("liters") and d.get("price") and not d.get("amount"):
            d["amount"] = round(d["liters"] * d["price"], 2)
        d["missing"] = [k for k in ("liters", "price", "amount") if not d.get(k)]
        d["warnings"] = receipt._warnings(d)
        st["stage"] = "confirm"
        return send(chat_id, summary_text(d), CONFIRM_BUTTONS)
    if st and st["stage"] == "mileage":
        if not re.fullmatch(r"\d{1,8}", text):
            return send(chat_id, "Введите пробег числом, например 124580, или 0.")
        if int(text) > 10_000_000:
            return send(chat_id, "Пробег должен быть не больше 10 000 000 км.")
        return finish(send, chat_id, int(text))
    send(chat_id, "Выберите действие.", menu.MENU)
