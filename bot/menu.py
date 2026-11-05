"""Button navigation and step-by-step input. Receipt drafts stay in handlers.pending."""
from datetime import date
from uuid import uuid4
import math

from . import api

def button(text, action):
    return {"text": text, "callback_data": action}

MENU = [
    [button("📷 Сфотографировать чек", "menu:photo")],
    [button("⛽ Добавить заправку", "menu:manual"), button("🔔 Напоминания", "menu:reminders")],
    [button("🚘 Мои автомобили", "menu:cars"), button("🔗 Привязать аккаунт", "menu:link")],
    [button("ℹ️ Помощь", "menu:help"), button("Отвязать аккаунт", "menu:unlink")],
]
BACK = [[button("🏠 Главное меню", "menu:home")]]
FIELDS = [[button("Литры", "field:liters"), button("Цена за литр", "field:price")],
          [button("Сумма", "field:amount"), button("АЗС", "field:place")],
          [button("Дата", "field:day"), button("Назад", "review")]]
MILEAGE = [[button("Пропустить пробег и сохранить", "skip")],
           [button("Отмена", "cancel")]]
LABELS = {"liters": "количество литров", "price": "цену за литр", "amount": "сумму",
          "place": "название АЗС", "day": "дату в формате ДД.ММ.ГГГГ"}

def handle(send, chat, action, handlers):
    if action == "home":
        handlers.pending.pop(chat, None)
        return send(chat, "CarLog · Главное меню\nВыберите действие.", MENU)
    if action == "photo":
        return send(chat, "Пришлите фото чека целиком. Я распознаю литры, цену и сумму.", BACK)
    if action == "help":
        return send(chat, "Привяжите аккаунт кнопкой ниже. Затем отправьте фото чека, проверьте данные и подтвердите заправку. Числа можно исправить по отдельности.", MENU)
    if action == "reminders":
        return handlers.on_text(send, chat, "/reminders")
    if action.startswith("done:"):
        api.complete_reminder(chat, int(action[5:]))
        return send(chat, "Напоминание отмечено выполненным.", BACK)
    if action == "cars":
        cars = api.cars(chat)
        return send(chat, "\n".join(f"🚘 {c['brand']} {c['model']} · {c['mileage']} км" for c in cars) or
                    "Сначала добавьте автомобиль в приложении.", BACK)
    if action == "unlink":
        return send(chat, "Отвязать этот Telegram от CarLog?",
                    [[button("Да, отвязать", "menu:unlink_yes")], BACK[0]])
    if action == "unlink_yes":
        return handlers.on_text(send, chat, "/unlink")
    if action == "link":
        handlers.pending[chat] = {"stage": "link_code", "draft": uuid4().hex[:12]}
        return send(chat, "Откройте настройки CarLog → Telegram → Получить код. Вставьте сюда 6 цифр. Это нужно один раз.", BACK)
    if action == "manual":
        api.cars(chat)
        handlers.pending[chat] = {"stage": "manual_liters", "draft": uuid4().hex[:12]}
        return send(chat, "Сколько литров заправлено? Введите число, например 40,5.", BACK)
    return handle(send, chat, "home", handlers)

def text(send, chat, value, handlers):
    state = handlers.pending.get(chat, {})
    stage = state.get("stage", "")
    if stage == "link_code":
        if len(value) != 6 or not value.isdigit():
            send(chat, "Введите 6 цифр кода из приложения.", BACK)
        else:
            handlers.on_text(send, chat, "/link " + value)
        return True
    if stage in ("manual_liters", "manual_price"):
        try:
            n = float(value.replace(",", "."))
            limit = 1000 if stage == "manual_liters" else 100000
            if not math.isfinite(n) or not 0 < n <= limit:
                raise ValueError
        except ValueError:
            send(chat, "Введите положительное число в допустимом диапазоне.", BACK)
            return True
        if stage == "manual_liters":
            state.update(stage="manual_price", liters=n)
            send(chat, "Какая цена за литр? Введите число, например 59,90.", BACK)
        else:
            handlers.on_text(send, chat, f"/fuel {state['liters']} {n}")
        return True
    if stage == "field":
        key = state["field"]
        try:
            edit = handlers.receipt.parse_edit(
                {"liters":"литры", "price":"цена", "amount":"сумма", "place":"азс", "day":"дата"}[key]+"="+value)
            state["data"].update(edit)
            d = state["data"]
            if key in ("liters", "price") and d.get("liters") and d.get("price"):
                d["amount"] = round(d["liters"] * d["price"], 2)
            if key == "amount" and d.get("liters"):
                d["price"] = round(d["amount"] / d["liters"], 2)
            d["missing"] = [k for k in ("liters","price","amount") if not d.get(k)]
            d["warnings"] = handlers.receipt._warnings(d)
            state["stage"] = "confirm"
            send(chat, handlers.summary_text(d), handlers.CONFIRM_BUTTONS)
        except ValueError as error:
            send(chat, str(error), BACK)
        return True
    return False
