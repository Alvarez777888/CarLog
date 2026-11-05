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

QWidget *MainWindow::carsPage()
{
    auto *page = new QWidget;

    auto *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(
        35, 30, 35, 30
    );

    layout->setSpacing(16);

    auto *title =
        new QLabel("Автомобили");

    title->setObjectName("pageTitle");

    layout->addWidget(title);

    auto *sub =
        new QLabel(
            "Выберите автомобиль для изменения или удаления"
        );

    sub->setObjectName("sub");

    layout->addWidget(sub);

    auto *buttons =
        new QHBoxLayout;

    auto *add =
        new QPushButton(
            "Добавить автомобиль"
        );

    auto *edit =
        new QPushButton(
            "Изменить"
        );

    auto *remove =
        new QPushButton(
            "Удалить"
        );

    add->setObjectName("darkBtn");
    edit->setObjectName("lightBtn");
    remove->setObjectName("lightBtn");

    add->setFocusPolicy(Qt::NoFocus);
    edit->setFocusPolicy(Qt::NoFocus);
    remove->setFocusPolicy(Qt::NoFocus);

    buttons->addWidget(add);
    buttons->addWidget(edit);
    buttons->addWidget(remove);
    buttons->addStretch();

    layout->addLayout(buttons);

    carCards =
        new QWidget;

    carCardsLayout =
        new QGridLayout(carCards);

    carCardsLayout->setContentsMargins(
        5, 5, 15, 15
    );

    carCardsLayout->setHorizontalSpacing(16);
    carCardsLayout->setVerticalSpacing(16);

    carCardsLayout->setColumnStretch(
        0,
        1
    );

    carCardsLayout->setColumnStretch(
        1,
        1
    );

    auto *scroll =
        new QScrollArea;

    scroll->setWidgetResizable(true);
    scroll->setFrameShape(
        QFrame::NoFrame
    );

    scroll->setWidget(carCards);

    layout->addWidget(
        scroll,
        1
    );

    connect(
        add,
        &QPushButton::clicked,
        this,
        [this]()
        {
            addCar();
        }
    );

    connect(
        edit,
        &QPushButton::clicked,
        this,
        [this]()
        {
            editCar();
        }
    );

    connect(
        remove,
        &QPushButton::clicked,
        this,
        [this]()
        {
            deleteCar();
        }
    );

    return page;
}


// ============================================================
// RECORDS PAGE
// ============================================================
