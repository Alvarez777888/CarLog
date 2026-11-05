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

QWidget *MainWindow::home()
{
    auto *page = new QWidget;

    auto *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(
        35, 30, 35, 30
    );

    layout->setSpacing(20);

    auto *title =
        new QLabel("Главная");

    title->setObjectName("pageTitle");

    layout->addWidget(title);

    auto *sub =
        new QLabel(
            "Обзор автомобиля и последних событий"
        );

    sub->setObjectName("sub");

    layout->addWidget(sub);

    auto *carSelectLabel =
        new QLabel("Автомобиль:");

    carSelectLabel->setStyleSheet(
        "background: transparent; font-weight: 600;"
    );

    auto *homeCarSelector =
        new QComboBox;

    homeCarSelector->setObjectName(
        "homeCarSelector"
    );

    homeCarSelector->setMinimumHeight(40);
    homeCarSelector->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
    );

    auto *carSelectorLayout =
        new QHBoxLayout;

    carSelectorLayout->setSpacing(10);
    carSelectorLayout->addWidget(carSelectLabel);
    carSelectorLayout->addWidget(homeCarSelector, 1);

    layout->addLayout(carSelectorLayout);

    connect(
        homeCarSelector,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this, homeCarSelector](int index)
        {
            if (index < 0)
                return;

            selectedCarId =
                homeCarSelector->itemData(index).toLongLong();

            refreshHome();
            renderCars();
        }
    );

    auto *grid =
        new QGridLayout;

    grid->setSpacing(15);

    homeMileage =
        new QLabel("Пробег\n—");

    homeFuel =
        new QLabel("Расход топлива\n—");

    homeExpenses =
        new QLabel("Расходы\n—");

    homeRecords =
        new QLabel("Записей\n—");

    homeMileage->setObjectName("card");
    homeFuel->setObjectName("card");
    homeExpenses->setObjectName("card");
    homeRecords->setObjectName("card");

    grid->addWidget(
        homeMileage,
        0,
        0
    );

    grid->addWidget(
        homeFuel,
        0,
        1
    );

    grid->addWidget(
        homeExpenses,
        1,
        0
    );

    grid->addWidget(
        homeRecords,
        1,
        1
    );

    layout->addLayout(grid);

    auto *section =
        new QFrame;

    section->setObjectName(
        "section"
    );

    auto *sectionLayout =
        new QVBoxLayout(section);

    sectionLayout->setContentsMargins(
        20, 20, 20, 20
    );

    auto *sectionTitle =
        new QLabel(
            "Быстрые действия"
        );

    sectionTitle->setStyleSheet(
        "background: transparent;"
        "font-size: 13pt;"
        "font-weight: 600;"
    );

    sectionLayout->addWidget(
        sectionTitle
    );

    auto *buttons =
        new QHBoxLayout;

    buttons->setSpacing(10);

    auto *fuel =
        new QPushButton(
            "Добавить заправку"
        );

    auto *expense =
        new QPushButton(
            "Добавить расход"
        );

    auto *cars =
        new QPushButton(
            "Автомобили"
        );

    fuel->setObjectName("darkBtn");
    expense->setObjectName("lightBtn");
    cars->setObjectName("lightBtn");

    fuel->setFocusPolicy(Qt::NoFocus);
    expense->setFocusPolicy(Qt::NoFocus);
    cars->setFocusPolicy(Qt::NoFocus);

    buttons->addWidget(fuel);
    buttons->addWidget(expense);
    buttons->addWidget(cars);
    buttons->addStretch();

    sectionLayout->addLayout(buttons);

    layout->addWidget(section);

    auto *info =
        new QFrame;

    info->setObjectName(
        "section"
    );

    auto *infoLayout =
        new QVBoxLayout(info);

    auto *infoTitle =
        new QLabel("Информация");

    infoTitle->setStyleSheet(
        "background: transparent;"
        "font-size: 13pt;"
        "font-weight: 600;"
    );

    auto *infoText =
        new QLabel(
            "Начните с добавления своего автомобиля.\n"
            "Заправки и напоминания доступны на всех ваших компьютерах."
        );

    infoText->setObjectName("sub");
    infoText->setWordWrap(true);

    infoLayout->addWidget(infoTitle);
    infoLayout->addWidget(infoText);

    layout->addWidget(info);

    layout->addStretch();

    connect(
        fuel,
        &QPushButton::clicked,
        this,
        [this]()
        {
            pages->setCurrentIndex(2);

            loadRecords(
                refuelTable,
                "Заправки"
            );
        }
    );

    connect(
        expense,
        &QPushButton::clicked,
        this,
        [this]()
        {
            pages->setCurrentIndex(3);

            loadRecords(
                expenseTable,
                "Расходы"
            );
        }
    );

    connect(
        cars,
        &QPushButton::clicked,
        this,
        [this]()
        {
            pages->setCurrentIndex(1);
            renderCars();
        }
    );

    return page;
}


// ============================================================
// CARS PAGE
// ============================================================
