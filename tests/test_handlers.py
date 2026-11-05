import unittest
from unittest import mock

from bot import handlers

CHECK = "Лукойл АЗС 5\n05.11.2026\nОбъем 40.00 л\nЦена 60.00\nИТОГО 2400.00\n"


class HandlersTest(unittest.TestCase):
    def setUp(self):
        handlers.pending.clear()
        self.out = []
        self.send = lambda chat, text, buttons=None: self.out.append((text, buttons))

    def test_receipt_to_saved_fuel_single_car(self):
        with mock.patch.object(handlers.api, "cars", return_value=[{"id": 3, "brand": "Toyota", "model": "Camry"}]), \
             mock.patch.object(handlers.api, "add_fuel", return_value={"liters": 40, "price": 60, "amount": 2400, "place": "Лукойл АЗС 5"}) as add:
            handlers.start_receipt(self.send, 1, CHECK)
            self.assertIn("Литры: 40 л", self.out[-1][0])
            handlers.on_callback(self.send, 1, "ok")
            self.assertIn("пробег", self.out[-1][0])
            handlers.on_text(self.send, 1, "1234")
            self.assertIn("Заправка сохранена", self.out[-1][0])
            kwargs = add.call_args.kwargs
            self.assertEqual((kwargs["car"], kwargs["mileage"], kwargs["liters"]), (3, 1234, 40.0))

    def test_several_cars_ask_choice(self):
        cars = [{"id": 1, "brand": "A", "model": "1"}, {"id": 2, "brand": "B", "model": "2"}]
        with mock.patch.object(handlers.api, "cars", return_value=cars):
            handlers.start_receipt(self.send, 1, CHECK)
            handlers.on_callback(self.send, 1, "ok")
            self.assertEqual(len(self.out[-1][1]), 2)
            handlers.on_callback(self.send, 1, "car:2")
            self.assertEqual(handlers.pending[1]["car"], 2)

    def test_edit_flow_and_cancel(self):
        handlers.start_receipt(self.send, 1, "Лукойл\nОбъем 40.00 л\n")
        handlers.on_callback(self.send, 1, "ok")                 # не хватает данных
        self.assertIn("Не хватает", self.out[-1][0])
        handlers.on_text(self.send, 1, "цена=50")
        self.assertIn("Сумма: 2000 ₽", self.out[-1][0])
        handlers.on_callback(self.send, 1, "cancel")
        self.assertNotIn(1, handlers.pending)

    def test_manual_fuel_and_bad_input(self):
        handlers.on_text(self.send, 1, "/fuel 10 50 Шелл")
        self.assertIn("Сумма: 500 ₽", self.out[-1][0])
        handlers.on_text(self.send, 1, "/fuel abc")
        self.assertIn("Формат", self.out[-1][0])

    def test_previous_receipt_button_cannot_cancel_new_receipt(self):
        handlers.start_receipt(self.send, 1, CHECK)
        old = handlers.telegram_buttons(1, handlers.CONFIRM_BUTTONS)[1][0]["callback_data"]
        handlers.start_receipt(self.send, 1, CHECK)
        handlers.telegram_callback(self.send, 1, old)
        self.assertIn(1, handlers.pending)
        self.assertIn("предыдущему чеку", self.out[-1][0])

    def test_unreadable_photo(self):
        handlers.start_receipt(self.send, 1, "asd")
        self.assertIn("Не удалось распознать", self.out[-1][0])


if __name__ == "__main__":
    unittest.main()

class MenuTest(unittest.TestCase):
    def setUp(self):
        handlers.pending.clear()
        self.out=[]
        self.send=lambda chat,text,buttons=None:self.out.append((text,buttons))

    def test_button_menu_manual_and_skip(self):
        from bot import menu
        with mock.patch.object(handlers.api,"cars",return_value=[{"id":3,"brand":"Toyota","model":"Camry"}]), \
             mock.patch.object(handlers.api,"add_fuel",return_value={"liters":40,"price":60,"amount":2400,"place":""}) as add:
            handlers.telegram_callback(self.send,1,"menu:manual")
            handlers.on_text(self.send,1,"40")
            handlers.on_text(self.send,1,"60")
            draft=handlers.pending[1]["draft"]
            handlers.telegram_callback(self.send,1,draft+"|ok")
            handlers.telegram_callback(self.send,1,draft+"|skip")
            self.assertEqual(add.call_args.kwargs["liters"],40)
            self.assertNotIn(1,handlers.pending)

    def test_button_edit_field(self):
        handlers.start_receipt(self.send,1,CHECK)
        handlers.on_callback(self.send,1,"edit")
        handlers.on_callback(self.send,1,"field:liters")
        handlers.on_text(self.send,1,"50")
        self.assertEqual(handlers.pending[1]["data"]["amount"],3000)
        self.assertEqual(handlers.pending[1]["stage"],"confirm")
