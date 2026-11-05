#include "formatting.hpp"

#include <QDate>
#include <QSettings>

QString currencyCode()
{
    QSettings s("CarLog", "CarLog");
    return s.value("currency", "RUB").toString();
}

QString distanceUnit()
{
    QSettings s("CarLog", "CarLog");
    return s.value("distance", "km").toString();
}

QString consumptionUnit()
{
    QSettings s("CarLog", "CarLog");
    return s.value("consumption", "l100").toString();
}

double currencyRate()
{
    const QString c = currencyCode();

    if (c == "USD")
        return 0.011;

    if (c == "EUR")
        return 0.010;

    return 1.0;
}

QString currencySymbol()
{
    const QString c = currencyCode();

    if (c == "USD")
        return "$";

    if (c == "EUR")
        return "€";

    return "₽";
}

QString money(double value)
{
    value *= currencyRate();

    return QString::number(value, 'f', 2) + " " + currencySymbol();
}

double displayDistance(double km)
{
    if (distanceUnit() == "mile")
        return km * 0.621371;

    return km;
}

QString distanceText(double km)
{
    const double value = displayDistance(km);

    if (distanceUnit() == "mile")
        return QString::number(value, 'f', 0) + " mi";

    return QString::number(value, 'f', 0) + " км";
}

QString displayDate(const QString &value)
{
    const QDate date = QDate::fromString(value, "yyyy-MM-dd");

    if (!date.isValid())
        return value;

    return date.toString("dd.MM.yyyy");
}

QString consumptionText(double litersPer100Km)
{
    if (litersPer100Km <= 0)
        return "—";

    if (consumptionUnit() == "mpg")
    {
        const double mpg = 235.214583 / litersPer100Km;
        return QString::number(mpg, 'f', 1) + " MPG";
    }

    return QString::number(litersPer100Km, 'f', 1) + " л/100 км";
}
