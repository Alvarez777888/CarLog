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

QWidget *MainWindow::statistics()
{
    auto *page = new QWidget;

    auto *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(
        35, 30, 35, 30
    );

    layout->setSpacing(18);

    auto *title =
        new QLabel("Статистика");

    title->setObjectName(
        "pageTitle"
    );

    layout->addWidget(title);

    auto *sub =
        new QLabel(
            "Расходы и общая информация"
        );

    sub->setObjectName("sub");

    layout->addWidget(sub);

    statsTotal =
        new QLabel("Всего\n—");

    statsFuel =
        new QLabel("Заправки\n—");

    statsService =
        new QLabel("Обслуживание\n—");

    statsOther =
        new QLabel("Другие расходы\n—");

    statsTotal->setObjectName("card");
    statsFuel->setObjectName("card");
    statsService->setObjectName("card");
    statsOther->setObjectName("card");

    auto *grid =
        new QGridLayout;

    grid->setSpacing(15);

    grid->addWidget(
        statsTotal,
        0,
        0
    );

    grid->addWidget(
        statsFuel,
        0,
        1
    );

    grid->addWidget(
        statsService,
        1,
        0
    );

    grid->addWidget(
        statsOther,
        1,
        1
    );

    layout->addLayout(grid);

    auto *refresh =
        new QPushButton(
            "Обновить статистику"
        );

    refresh->setObjectName(
        "darkBtn"
    );

    refresh->setFocusPolicy(
        Qt::NoFocus
    );

    layout->addWidget(
        refresh,
        0,
        Qt::AlignLeft
    );

    layout->addStretch();

    connect(
        refresh,
        &QPushButton::clicked,
        this,
        [this]()
        {
            refreshStats();
        }
    );

    return page;
}


// ============================================================
// SETTINGS
// ============================================================
