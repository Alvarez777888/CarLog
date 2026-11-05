#include "mainwindow.hpp"
#include "formatting.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QStyle>
#include <QComboBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QTableWidget>
#include <QStackedWidget>

QWidget *MainWindow::sidebar()
{
    auto *widget = new QWidget;

    widget->setObjectName("sidebar");
    widget->setFixedWidth(255);

    auto *layout =
        new QVBoxLayout(widget);

    layout->setContentsMargins(
        24, 40, 20, 25
    );

    layout->setSpacing(3);

    auto *title =
        new QLabel("CarLog");

    title->setObjectName("sideTitle");

    layout->addWidget(title);

    auto *subtitle =
        new QLabel("Автомобильный дневник");

    subtitle->setObjectName("sideSub");

    layout->addWidget(subtitle);

    layout->addSpacing(40);

    const QStringList names = {
        "Главная",
        "Автомобили",
        "Заправки",
        "Расходы",
        "Обслуживание",
        "Напоминания",
        "Статистика",
        "Настройки"
    };

    for (int i = 0;
         i < names.size();
         ++i)
    {
        auto *button =
            navButton(names[i]);

        if (i == 0)
            button->setProperty(
                "active",
                true
            );

        layout->addWidget(button);

        connect(
            button,
            &QPushButton::clicked,
            this,
            [this, i, button]()
            {
                const auto buttons =
                    findChildren<QPushButton *>(
                        "nav"
                    );

                for (auto *b : buttons)
                {
                    b->setProperty(
                        "active",
                        b == button
                    );

                    b->style()->unpolish(b);
                    b->style()->polish(b);
                    b->update();
                }

                pages->setCurrentIndex(i);

                if (i == 0)
                {
                    refreshHome();
                }
                else if (i == 1)
                {
                    loadCars();
                }
                else if (i == 2)
                {
                    loadRecords(
                        refuelTable,
                        "Заправки"
                    );
                }
                else if (i == 3)
                {
                    loadRecords(
                        expenseTable,
                        "Расходы"
                    );
                }
                else if (i == 4)
                {
                    loadRecords(
                        serviceTable,
                        "Обслуживание"
                    );
                }
                else if (i == 5)
                {
                    loadRecords(
                        reminderTable,
                        "Напоминания"
                    );
                }
                else if (i == 6)
                {
                    refreshStats();
                }
            }
        );
    }

    layout->addStretch();

    auto *footer =
        new QLabel(
            "CarLog\n"
            "Ваши данные на всех устройствах"
        );

    footer->setObjectName("sideFoot");

    layout->addWidget(footer);

    return widget;
}


// ============================================================
// NAV BUTTON
// ============================================================

QPushButton *MainWindow::navButton(
    const QString &text)
{
    auto *button =
        new QPushButton(text);

    button->setObjectName("nav");

    button->setCursor(
        Qt::PointingHandCursor
    );

    button->setFocusPolicy(
        Qt::NoFocus
    );

    return button;
}


// ============================================================
// HOME
// ============================================================
