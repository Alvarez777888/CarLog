"""Телеграм-бот Car Log. Запуск: python run_bot.py"""
import logging
import os
import tempfile
import threading
import time

import requests

from . import api, config, handlers, receipt, telegram, menu

log = logging.getLogger("carlog.bot")


def safe(send_fn, chat_id, func, *args):
    """Любая ошибка в одном сообщении не должна ронять бота."""
    try:
        func(send_fn, chat_id, *args)
    except api.ServerError as e:
        if e.code == "not_linked":
            send_fn(chat_id, "Сначала подключите аккаунт кнопкой «Привязать аккаунт».", menu.MENU)
        else:
            send_fn(chat_id, f"Ошибка: {e.message}")
    except Exception:  # noqa: BLE001
        log.warning("Ошибка обработки")
        send_fn(chat_id, "Что-то пошло не так. Попробуйте ещё раз.")


def send_dialog(chat_id, text, buttons=None):
    if buttons is None:
        buttons = menu.MENU
    if buttons:
        buttons = handlers.telegram_buttons(chat_id, buttons)
    return telegram.send(chat_id, text, buttons)


def handle_photo(send, chat_id, file_id, source_key):
    api.cars(chat_id)

    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "receipt.jpg")
        telegram.download_photo(file_id, path)
        try:
            text = receipt.ocr_image(path, config.OCR_LANG)
        except Exception as error:  # noqa: BLE001
            log.exception("Ошибка распознавания")
            return send(chat_id, "Не удалось распознать чек. Попробуйте более чёткое фото или выберите ручное добавление.")
    handlers.start_receipt(send, chat_id, text, source_key)


def send_outbox():
    """Сообщения от сервера (коды восстановления пароля): отправляем и отмечаем как доставленные."""
    for msg in api.outbox():
        try:
            telegram.send(msg["chat_id"], msg["text"])
        except Exception:  # noqa: BLE001
            log.warning("Не удалось отправить сообщение сервера")
            continue
        api.outbox_sent(msg["id"])


def outbox_loop():
    while True:
        try:
            send_outbox()
        except Exception:  # noqa: BLE001
            log.warning("Ошибка очереди сообщений")
        time.sleep(config.OUTBOX_CHECK_SECONDS)


def reminders_loop():
    while True:
        try:
            for item in api.due_reminders():
                text = (f"🔔 Напоминание: {item['description']}\n"
                        f"Автомобиль: {item['brand']} {item['model']}\n"
                        f"Дата: {handlers.fmt_date(item['day'])}")
                try:
                    telegram.send(item["chat_id"], text, [[menu.button("✅ Выполнено", f"menu:done:{item['id']}")], menu.BACK[0]])
                    api.mark_notified(item)
                except Exception:
                    log.warning("Не удалось доставить напоминание %s", item["id"])
        except Exception:  # noqa: BLE001
            log.warning("Ошибка проверки напоминаний")
        time.sleep(config.REMINDER_CHECK_SECONDS)


def run():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(levelname)s %(message)s")
    if not config.BOT_TOKEN or not config.BOT_SECRET:
        raise SystemExit("Задайте CARLOG_BOT_TOKEN и CARLOG_BOT_SECRET (см. README).")
    threading.Thread(target=reminders_loop, daemon=True).start()
    threading.Thread(target=outbox_loop, daemon=True).start()
    offset = 0
    log.info("Бот запущен")
    while True:
        try:
            batch = telegram.updates(offset)
            if not batch:
                time.sleep(1)
            for upd in batch:
                offset = upd["update_id"] + 1
                if "callback_query" in upd:
                    cb = upd["callback_query"]
                    telegram.answer_callback(cb["id"])
                    chat = (cb.get("message") or {}).get("chat") or {}
                    if chat.get("type") == "private":
                        safe(send_dialog, chat["id"], handlers.telegram_callback, cb.get("data", ""))
                    continue
                msg = upd.get("message") or {}
                chat_id = (msg.get("chat") or {}).get("id")
                if not chat_id or msg["chat"].get("type") != "private":
                    continue
                photo = msg["photo"][-1] if msg.get("photo") else None
                document = msg.get("document") or {}
                if not photo and document.get("mime_type") in {"image/jpeg", "image/png"}:
                    photo = document
                if photo:
                    safe(send_dialog, chat_id, handle_photo, photo["file_id"], photo["file_unique_id"])
                elif msg.get("text"):
                    safe(send_dialog, chat_id, handlers.on_text, msg["text"])
        except telegram.TelegramError as error:
            log.warning("Telegram HTTP %s. Повтор через 5 секунд.", error.code)
            if error.code == 409:
                log.warning("Проверьте, что запущена одна копия бота и отключён webhook.")
            time.sleep(5)
        except requests.RequestException as error:
            # Do not log the URL, which contains the bot token.
            log.warning("Ошибка сети (%s). Повтор через 5 секунд.", type(error).__name__)
            time.sleep(5)
        except Exception:  # noqa: BLE001
            log.warning("Ошибка обработки обновлений. Повтор через 5 секунд.")
            time.sleep(5)
