import unittest

from bot import receipt

CHECK1 = """ООО ЛУКОЙЛ-Югнефтепродукт
АЗС №12 г. Новосибирск
ККМ 00012345  ИНН 7700000000
05.11.2026 14:32
Топливо: АИ-95
Объем: 42,50 л
Цена: 58,90
Сумма: 2503,25
НДС 20%: 417,21
ИТОГО: 2503,25
"""

CHECK2 = """Газпромнефть АЗС 77
12.10.26
ДТ
Количество 35.000
Цена за литр 62.40
К оплате 2184.00
"""

CHECK3 = """Роснефть
Заправлено 30.2 л
2200 руб
"""


class ReceiptTest(unittest.TestCase):
    def test_full_check(self):
        r = receipt.parse_receipt(CHECK1)
        self.assertEqual(r["liters"], 42.5)
        self.assertEqual(r["price"], 58.9)
        self.assertEqual(r["amount"], 2503.25)
        self.assertEqual(r["day"], "2026-11-05")
        self.assertIn("ЛУКОЙЛ", r["place"].upper())
        self.assertEqual(r["fuel"], "АИ-95")
        self.assertEqual(r["missing"], [])
        self.assertEqual(r["warnings"], [])

    def test_second_format(self):
        r = receipt.parse_receipt(CHECK2)
        self.assertEqual((r["liters"], r["price"], r["amount"]), (35.0, 62.4, 2184.0))
        self.assertEqual(r["day"], "2026-10-12")
        self.assertEqual(r["fuel"], "ДТ")
        self.assertIn("ГАЗПРОМНЕФТЬ", r["place"].upper())

    def test_missing_price_is_computed(self):
        r = receipt.parse_receipt("Роснефть\nОбъем 30.00 л\nИтого 1800.00\n")
        self.assertEqual(r["price"], 60.0)

    def test_garbage(self):
        r = receipt.parse_receipt("qwe asd zxc")
        self.assertEqual(set(r["missing"]), {"liters", "price", "amount"})

    def test_edit_parsing(self):
        e = receipt.parse_edit("литры=40,5 цена=59.9 азс=Лукойл №3 дата=01.11.2026")
        self.assertEqual(e, {"liters": 40.5, "price": 59.9, "place": "Лукойл №3", "day": "2026-11-01"})
        with self.assertRaises(ValueError):
            receipt.parse_edit("привет")
        with self.assertRaises(ValueError):
            receipt.parse_edit("литры=много")


if __name__ == "__main__":
    unittest.main()
