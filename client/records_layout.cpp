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

QWidget *MainWindow::recordsPage(
    const QString &type)
{
    auto *page = new QWidget;

    auto *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(
        35, 30, 35, 30
    );

    layout->setSpacing(15);

    auto *title =
        new QLabel(type);

    title->setObjectName(
        "pageTitle"
    );

    layout->addWidget(title);

    auto *sub =
        new QLabel(
            "История записей"
        );

    sub->setObjectName("sub");

    layout->addWidget(sub);

    auto *buttons =
        new QHBoxLayout;

    auto *add =
        new QPushButton("Добавить");

    auto *edit =
        new QPushButton("Изменить");

    auto *remove =
        new QPushButton("Удалить");

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

    auto *table =
        new QTableWidget;

    table->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    table->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    table->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    table->setAlternatingRowColors(
        true
    );

    table->horizontalHeader()
        ->setStretchLastSection(true);

    if (type == "Заправки")
    {
        refuelTable = table;

        table->setColumnCount(8);

        table->setHorizontalHeaderLabels({
            "Дата",
            "Автомобиль",
            "Литры",
            "Цена",
            "Сумма",
            "Пробег",
            "Место",
            "Комментарий"
        });
    }
    else if (type == "Расходы")
    {
        expenseTable = table;

        table->setColumnCount(7);

        table->setHorizontalHeaderLabels({
            "Дата",
            "Категория",
            "Описание",
            "Сумма",
            "Пробег",
            "Автомобиль",
            "Комментарий"
        });
    }
    else if (type == "Обслуживание")
    {
        serviceTable = table;

        table->setColumnCount(7);

        table->setHorizontalHeaderLabels({
            "Дата",
            "Описание",
            "Пробег",
            "Сумма",
            "Место",
            "Автомобиль",
            "Комментарий"
        });
    }
    else
    {
        reminderTable = table;

        table->setColumnCount(4);

        table->setHorizontalHeaderLabels({
            "Дата",
            "Напоминание",
            "Автомобиль",
            "Статус"
        });
    }

    layout->addWidget(table);

    connect(
        add,
        &QPushButton::clicked,
        this,
        [this, type]()
        {
            addRecord(type);
        }
    );

    connect(
        edit,
        &QPushButton::clicked,
        this,
        [this, type]()
        {
            editRecord(type);
        }
    );

    connect(
        remove,
        &QPushButton::clicked,
        this,
        [this, type]()
        {
            deleteRecord(type);
        }
    );

    if (type == "Напоминания") {
        auto *toggle = new QPushButton("Изменить статус");
        toggle->setObjectName("lightBtn");
        buttons->insertWidget(3, toggle);
        connect(toggle, &QPushButton::clicked, this, [this] { toggleReminder(); });
    }
    return page;
}


// ============================================================
// STATISTICS
// ============================================================
