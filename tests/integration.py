"""Run against a disposable CarLog instance. Tests create their own accounts."""
import os
import re
from uuid import uuid4

import requests

from bot import receipt

BASE = os.environ.get("CARLOG_TEST_URL", "http://127.0.0.1:8000")
BOT = {"X-Bot-Secret": os.environ["CARLOG_BOT_SECRET"]}
PREFIX = "test_" + uuid4().hex[:10]
PASSWORD = "old-secret-123"


def call(verb, path, status=200, headers=None, **data):
    r = requests.request(verb, BASE + path, headers=headers, json=data or None, timeout=15)
    assert r.status_code == status, (path, r.status_code, r.text)
    return r.json()


def get(path, headers):
    return call("GET", path, headers=headers)


def post(path, headers=None, status=200, **fields):
    return call("POST", path, status, headers, **fields)


def session(name, password=PASSWORD):
    result = post("/api/login", login=name, password=password)
    return {"Authorization": "Bearer " + result["token"]}


def main():
    health = call("GET", "/api/health")
    assert health["status"] == "ok", health
    name, other = PREFIX, PREFIX + "_other"
    post("/api/register", status=201, login=name, password=PASSWORD)
    post("/api/register", status=409, login=name, password=PASSWORD)
    post("/api/register", status=201, login=other, password=PASSWORD)
    desktop, second_device, stranger = session(name), session(name), session(other)
    car = post("/api/cars", desktop, 201, brand="Toyota", model="Camry", year=2019, request_id="car-request")["id"]
    assert post("/api/cars", desktop, 201, brand="Toyota", model="Camry", year=2019, request_id="car-request")["id"] == car
    assert len(get("/api/cars", desktop)) == 1
    assert call("DELETE", f"/api/cars/{car}", 404, stranger)["code"] == "not_found"
    code = post("/api/telegram/link-code", desktop)["code"]
    chat = int(uuid4().hex[:15], 16)
    post("/api/bot/link", BOT, chat_id=chat, code=code)
    post("/api/bot/link", BOT, 400, chat_id=chat, code=code)
    call("GET", "/api/bot/cars", 403)
    assert get(f"/api/bot/cars?chat_id={chat}", BOT)[0]["id"] == car

    check = receipt.parse_receipt("АЗС 1\nАИ-95\n40.50 x 59.90\nИТОГО 2425.95\n04.11.2026")
    fields = {key: check[key] for key in ("liters", "price", "amount", "day", "place")}
    fuel = post("/api/bot/fuel", BOT, 201, chat_id=chat, car=car, mileage=1234, source_key="check1", **fields)
    repeat = post("/api/bot/fuel", BOT, 201, chat_id=chat, car=car, mileage=1234, source_key="check1", **fields)
    assert fuel == repeat and (fuel["liters"], fuel["price"], fuel["amount"]) == (40.5, 59.9, 2425.95)
    assert len(get("/api/records?kind=fuel", second_device)) == 1
    assert get("/api/records?kind=fuel", stranger) == []
    assert get("/api/cars", second_device)[0]["mileage"] == 1234
    assert get("/api/stats/totals", desktop)["fuel"] == 2425.95
    assert get(f"/api/stats/summary?car={car}", desktop)["records_count"] == 1

    entry = dict(car=car,kind="expense",day="2026-11-04",amount=100,description="Test",request_id="entry-request")
    record = post("/api/records", desktop, 201, **entry)["id"]
    assert post("/api/records", desktop, 201, **entry)["id"] == record
    exported = get("/api/account/export", desktop)
    assert exported["login"] == name and len(exported["cars"]) == 1 and len(exported["records"]) == 2
    assert get("/api/account/export", stranger)["cars"] == []
    call("DELETE", f"/api/records/{record}", 200, desktop)

    reminder = post("/api/records", desktop, 201, car=car, kind="reminder", day="2000-01-01",
                    description="Замена масла")["id"]
    due = next(r for r in get("/api/bot/reminders/due", BOT) if r["id"] == reminder and r["chat_id"] == chat)
    assert due["stage"] == 2
    post(f"/api/bot/reminders/{reminder}/notified", BOT, **{k: due[k] for k in ("chat_id", "version", "stage")})
    assert all(r["id"] != reminder for r in get("/api/bot/reminders/due", BOT))
    call("PUT", f"/api/records/{reminder}", 200, second_device, day="2000-01-02", description="Проверить масло")
    assert any(r["id"] == reminder and r["version"] > due["version"] for r in get("/api/bot/reminders/due", BOT))
    post(f"/api/records/{reminder}/toggle", desktop)
    assert all(r["id"] != reminder for r in get("/api/bot/reminders/due", BOT))

    call("GET", "/", 404)
    call("GET", "/app/fuel", 404)

    post("/api/recovery/request", login_or_email=name, method="telegram")
    message = next(m for m in get("/api/bot/outbox", BOT) if m["chat_id"] == chat)
    code = re.search(r"\b[0-9]{6}\b", message["text"])[0]
    reset = post("/api/recovery/verify", login_or_email=name, method="telegram", code=code)["reset_token"]
    post("/api/recovery/reset", reset_token=reset, new_password="new-secret-123")
    call("GET", "/api/cars", 401, second_device)
    assert get("/api/cars", session(name, "new-secret-123"))[0]["id"] == car
    post("/api/account/import", stranger, **exported)
    imported=get("/api/account/export",stranger)
    assert len(imported["cars"])==1 and len(imported["records"])==2
    assert post("/api/account/import",stranger,**exported)["repeated"]
    assert len(get("/api/cars",stranger))==1
    broken=dict(exported)
    broken["records"]=[{"car":999999999,"kind":"fuel","day":"2026-11-04","liters":1,"price":1}]
    post("/api/account/import",stranger,400,**broken)
    assert len(get("/api/cars",stranger))==1
    h=session(name,"new-secret-123")
    old_session=session(name,"new-secret-123")
    post("/api/account/password",h,old_password="new-secret-123",new_password="changed-secret-123")
    call("GET","/api/cars",401,old_session)
    assert get("/api/account",h)["login"]==name
    print("OK: API, PostgreSQL, OCR parsing, receipt retries, reminders, API-only server, recovery")


if __name__ == "__main__":
    main()
