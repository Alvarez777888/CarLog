"""Настройки бота из переменных окружения."""
import os

BOT_TOKEN = os.environ.get("CARLOG_BOT_TOKEN", "")          # токен от @BotFather
SERVER_URL = os.environ.get("CARLOG_SERVER_URL", "http://127.0.0.1:8000").rstrip("/")
BOT_SECRET = os.environ.get("CARLOG_BOT_SECRET", "")        # тот же ключ, что у сервера
OCR_LANG = os.environ.get("CARLOG_OCR_LANG", "rus+eng")
REMINDER_CHECK_SECONDS = int(os.environ.get("CARLOG_REMINDER_CHECK", "60"))
OUTBOX_CHECK_SECONDS = int(os.environ.get("CARLOG_OUTBOX_CHECK", "3"))  # как часто забирать коды восстановления
