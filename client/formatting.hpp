#pragma once
// Настройки отображения и форматирование значений.
// Не зависит от окон: используется интерфейсом для вывода сумм, пробега и дат.
#include <QString>

QString currencyCode();
QString distanceUnit();
QString consumptionUnit();
double  currencyRate();
QString currencySymbol();

QString money(double value);               // сумма в выбранной валюте
double  displayDistance(double km);        // км -> выбранные единицы
QString distanceText(double km);
QString displayDate(const QString &value); // yyyy-MM-dd -> dd.MM.yyyy
QString consumptionText(double litersPer100Km);
