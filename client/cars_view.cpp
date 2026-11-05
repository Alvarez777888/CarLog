#include "mainwindow.hpp"
#include "mobile.hpp"
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

void MainWindow::renderCars()
{
    if (!carCards || !carCardsLayout)
        return;

    while (QLayoutItem *item =
               carCardsLayout->takeAt(0))
    {
        if (item->widget())
        {
            delete item->widget();
        }

        delete item;
    }

    int index = 0;

    for (const auto &value : cars)
    {
        const auto data = value.toObject();
        const qint64 id =
            static_cast<qint64>(data.value("id").toDouble());

        const QString brand =
            data.value("brand").toString();

        const QString model =
            data.value("model").toString();

        const int year =
            data.value("year").toInt();

        const QString fuel =
            data.value("fuel").toString();

        const double engine =
            data.value("engine").toDouble();

        const double mileage =
            data.value("mileage").toDouble();

        auto *card =
            new QToolButton(carCards);

        card->setObjectName(
            "carButton"
        );

        card->setCursor(
            Qt::PointingHandCursor
        );

        card->setFocusPolicy(
            Qt::NoFocus
        );

        card->setToolButtonStyle(
            Qt::ToolButtonTextOnly
        );

        // Карточка сохраняет все исходные данные автомобиля,
        // но показывает их более компактно и читаемо.
        card->setText(
            brand
            + " "
            + model
            + "\n"
            + "────────────────────────"
            + "\n"
            + "Год выпуска: "
            + QString::number(year)
            + "\n"
            + "Топливо: "
            + fuel
            + "\n"
            + "Двигатель: "
            + QString::number(
                engine,
                'f',
                1
            )
            + " л"
            + "\n"
            + "Пробег: "
            + distanceText(mileage)
            + "\n"
            + "\n"
            + "Нажмите, чтобы выбрать автомобиль"
        );

        card->setMinimumSize(
            0,
            215
        );

        card->setMaximumHeight(
            215
        );

        card->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Fixed
        );

        card->setProperty(
            "carId",
            id
        );

        card->setProperty(
            "selected",
            id == selectedCarId
        );

        /*
         * Выбор автомобиля.
         */
        connect(
            card,
            &QToolButton::clicked,
            this,
            [this, card]()
            {
                selectedCarId =
                    card->property(
                        "carId"
                    ).toLongLong();

                const auto cards =
                    carCards->findChildren<
                        QToolButton *
                    >(
                        "carButton"
                    );

                for (auto *other : cards)
                {
                    const bool selected =
                        other->property(
                            "carId"
                        ).toLongLong()
                        == selectedCarId;

                    other->setProperty(
                        "selected",
                        selected
                    );

                    other->style()->unpolish(
                        other
                    );

                    other->style()->polish(
                        other
                    );

                    other->update();
                }
            }
        );

        const int row =
            mobile_ui() ? index : index / 2;

        const int column =
            mobile_ui() ? 0 : index % 2;

        carCardsLayout->addWidget(
            card,
            row,
            column
        );

        ++index;
    }

    /*
     * Если автомобилей нет.
     */
    if (index == 0)
    {
        auto *empty =
            new QLabel(
                "Автомобилей пока нет.\n\n"
                "Нажмите «Добавить автомобиль», "
                "чтобы создать первую запись.",
                carCards
            );

        empty->setAlignment(
            Qt::AlignCenter
        );

        empty->setWordWrap(true);

        empty->setMinimumHeight(180);

        empty->setStyleSheet(
            "QLabel {"
            "background: white;"
            "color: #6b7484;"
            "border: 1px solid #e4e8ef;"
            "border-radius: 14px;"
            "padding: 30px;"
            "font-family: 'DejaVu Sans';"
            "font-size: 10pt;"
            "}"
        );

        carCardsLayout->addWidget(
            empty,
            0,
            0,
            1,
            mobile_ui() ? 1 : 2
        );
    }

    /*
     * Две равные колонки.
     */
    carCardsLayout->setColumnStretch(
        0,
        1
    );

    carCardsLayout->setColumnStretch(
        1,
        mobile_ui() ? 0 : 1
    );

    /*
     * Карточки всегда начинаются сверху.
     */
    carCardsLayout->setAlignment(
        Qt::AlignTop
        | Qt::AlignLeft
    );

    carCardsLayout->setContentsMargins(
        5,
        5,
        15,
        20
    );

    carCardsLayout->setHorizontalSpacing(
        16
    );

    carCardsLayout->setVerticalSpacing(
        16
    );

    carCards->adjustSize();

    auto *homeCarSelector =
        findChild<QComboBox *>("homeCarSelector");

    if (homeCarSelector)
    {
        homeCarSelector->blockSignals(true);
        homeCarSelector->clear();

        for (const auto &entry : cars) {
            const auto car = entry.toObject();
            homeCarSelector->addItem(car.value("brand").toString()+" "+car.value("model").toString(),
                                     QVariant::fromValue(static_cast<qint64>(car.value("id").toDouble())));
        }

        if (homeCarSelector->count() > 0)
        {
            int selectorIndex = homeCarSelector->findData(selectedCarId);
            if (selectorIndex < 0)
            {
                selectorIndex = 0;
                selectedCarId = homeCarSelector->itemData(0).toLongLong();
            }
            homeCarSelector->setCurrentIndex(selectorIndex);
        }
        else
        {
            selectedCarId = -1;
        }

        homeCarSelector->blockSignals(false);
    }
}


// ============================================================
// LOAD RECORDS
// ============================================================