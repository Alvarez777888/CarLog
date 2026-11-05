"""Разбор текста кассового чека АЗС: литры, цена, сумма, АЗС, дата.
Чистые функции без сети и без Tesseract, поэтому легко проверяются тестами."""
import math
import re
from datetime import date, datetime

NUM = r"((?:\d{1,3}(?:[ \u00a0\u202f]\d{3}){1,2}|\d{1,9})(?:[.,]\d{1,3})?)"
BRANDS = r"азс|лукойл|lukoil|роснефть|rosneft|газпромнефть|газпром|gazprom|shell|шелл|татнефть|тнк|bp\b|neste|несте|teboil|трасса|ofi|сургутнефтегаз|башнефть|опти|несте"
FUELS = r"аи-?\s?9[58]|аи-?\s?92|аи-?\s?100|ai-?\s?9[258]|дт\b|дизел\w*|пропан|метан|газ\b"


def _to_float(s):
    return float("".join(s.split()).replace(",", "."))


def _after_keywords(lines, keywords, skip=None):
    """Числа, стоящие после ключевых слов в строке."""
    found = []
    pattern = re.compile(rf"(?:{keywords})[^\d\n]{{0,25}}{NUM}", re.IGNORECASE)
    for line in lines:
        if skip and re.search(skip, line, re.IGNORECASE):
            continue
        for m in pattern.finditer(line):
            found.append(_to_float(m.group(1)))
    return found


def _liters_candidates(lines):
    vals = _after_keywords(lines, r"объ[её]м|кол-?во|количество|литр\w*|заправлено|отпущено|volume")
    for line in lines:
        for m in re.finditer(rf"{NUM}\s*(?:л\b|l\b|литр)", line, re.IGNORECASE):
            vals.append(_to_float(m.group(1)))
        match = re.search(rf"{NUM}\s*[xх×*]\s*{NUM}", line, re.IGNORECASE)
        if match:
            vals.append(_to_float(match.group(1)))
    return vals


def _price_candidates(lines):
    vals = _after_keywords(lines, r"цена|price|руб\s*/\s*л|₽\s*/\s*л")
    for line in lines:
        for m in re.finditer(rf"{NUM}\s*(?:руб|р|₽)?\s*/\s*л", line, re.IGNORECASE):
            vals.append(_to_float(m.group(1)))
        m = re.search(rf"{NUM}\s*[xх×*]\s*{NUM}", line, re.IGNORECASE)  # «42.5 x 58.9»
        if m:
            vals.append(_to_float(m.group(2)))
    return vals


def _amount_candidates(lines):
    return _after_keywords(lines, r"итого|сумма|к оплате|всего|total|стоимость|оплачено", skip=r"ндс|vat")


def _pick(liters, prices, amounts):
    """Выбирает сочетание, где литры * цена ≈ сумма; иначе берёт первые найденные."""
    best, best_err = None, None
    for l in liters:
        for p in prices:
            for a in amounts:
                err = abs(l * p - a)
                if err <= max(1.0, a * 0.01) and (best_err is None or err < best_err):
                    best, best_err = (l, p, a), err
    if best:
        return best
    return (liters[0] if liters else None, prices[0] if prices else None, amounts[-1] if amounts else None)


def _find_date(text):
    for m in re.finditer(r"(\d{2})[./-](\d{2})[./-](\d{4}|\d{2})", text):
        d, mth, y = int(m.group(1)), int(m.group(2)), int(m.group(3))
        y = y + 2000 if y < 100 else y
        try:
            return date(y, mth, d).isoformat()
        except ValueError:
            continue
    return None


def _find_place(lines):
    for line in lines:
        if re.search(BRANDS, line, re.IGNORECASE):
            return re.sub(r"\s+", " ", line).strip()[:60]
    for line in lines:
        if re.search(r"[A-Za-zА-Яа-я]{3,}", line):
            return re.sub(r"\s+", " ", line).strip()[:60]
    return ""


def parse_receipt(text):
    """Возвращает словарь: liters, price, amount, place, day, fuel, missing (что не нашли)."""
    lines = [ln.strip() for ln in text.splitlines() if ln.strip()]
    receipt_day = _find_date(text)
    liters, price, amount = _pick(_liters_candidates(lines), _price_candidates(lines), _amount_candidates(lines))

    if liters and price and not amount:
        amount = round(liters * price, 2)
    elif liters and amount and not price:
        price = round(amount / liters, 2)
    elif price and amount and not liters:
        liters = round(amount / price, 2)

    fuel_match = re.search(FUELS, text, re.IGNORECASE)
    result = {
        "liters": liters, "price": price, "amount": amount,
        "place": _find_place(lines),
        "day": receipt_day or date.today().isoformat(),
        "day_from_receipt": receipt_day is not None,
        "fuel": fuel_match.group(0).upper().replace(" ", "") if fuel_match else "",
    }
    result["missing"] = [k for k in ("liters", "price", "amount") if not result[k]]
    result["warnings"] = _warnings(result)
    return result


def _warnings(r):
    out = []
    if r["liters"] and not 1 <= r["liters"] <= 200:
        out.append("Странное количество литров.")
    if r["price"] and not 20 <= r["price"] <= 300:
        out.append("Странная цена за литр.")
    if r["liters"] and r["price"] and r["amount"] and abs(r["liters"] * r["price"] - r["amount"]) > max(1.0, r["amount"] * 0.02):
        out.append("Литры × цена не совпадают с суммой.")
    return out


def parse_edit(text):
    """Разбор исправления вида «литры=42.5 цена=58.9 сумма=2503 азс=Лукойл дата=05.11.2026»."""
    keys = {"литры": "liters", "л": "liters", "цена": "price", "сумма": "amount", "азс": "place", "дата": "day"}
    out = {}
    for m in re.finditer(r"(литры|цена|сумма|азс|дата|л)\s*=\s*([^=]+?)(?=\s+(?:литры|цена|сумма|азс|дата|л)\s*=|$)", text.strip(), re.IGNORECASE):
        key, value = keys[m.group(1).lower()], m.group(2).strip()
        if key in ("liters", "price", "amount"):
            try:
                n = _to_float(value)
                if not math.isfinite(n) or n <= 0:
                    raise ValueError
                out[key] = n
            except ValueError:
                raise ValueError(f"«{value}» — не число.")
        elif key == "day":
            d = _find_date(value)
            if not d:
                raise ValueError("Дата должна быть вида ДД.ММ.ГГГГ.")
            out[key] = d
        else:
            out[key] = value[:60]
    if not out:
        raise ValueError("Не понял. Пример: литры=42.5 цена=58.9 азс=Лукойл")
    return out


def ocr_image(path, lang="rus+eng"):
    """Распознаёт текст на фото (нужен установленный Tesseract)."""
    import pytesseract
    from PIL import Image, ImageOps

    with Image.open(path) as photo:
        if photo.width * photo.height > 30_000_000:
            raise ValueError("Receipt image is too large")
        img = ImageOps.grayscale(ImageOps.exif_transpose(photo))
    img = ImageOps.autocontrast(img)
    if img.width < 1000:  # маленькие фото плохо распознаются
        k = 1000 / img.width
        img = img.resize((int(img.width * k), int(img.height * k)))
    img.thumbnail((2400, 3600), Image.Resampling.LANCZOS)
    text = pytesseract.image_to_string(img, lang=lang, config="--psm 6", timeout=20)
    missing = len(parse_receipt(text)["missing"])
    if not missing:
        return text
    try:
        sparse = pytesseract.image_to_string(img, lang=lang, config="--psm 11", timeout=15)
    except RuntimeError:
        if text.strip():
            return text
        raise
    return sparse if len(parse_receipt(sparse)["missing"]) < missing else text
