#include "mainwindow.hpp"
#include "formatting.hpp"
#include "pages.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSettings>
#include <QMessageBox>
#include <QScrollArea>
QWidget *MainWindow::settings()
{
    auto *page = new QWidget;

    auto *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(
        35, 30, 35, 30
    );

    layout->setSpacing(18);

    auto *title =
        new QLabel("Настройки");

    title->setObjectName(
        "pageTitle"
    );

    layout->addWidget(title);

    auto *sub =
        new QLabel(
            "Отображение данных и ваш аккаунт"
        );

    sub->setObjectName("sub");

    layout->addWidget(sub);

    auto *account = new QGroupBox("Управление аккаунтом");
    auto *accountLayout = new QVBoxLayout(account);
    accountLayout->addWidget(account_page(api));
    layout->addWidget(account);

    auto *box =
        new QGroupBox(
            "Основные параметры"
        );

    auto *form =
        new QFormLayout(box);

    form->setSpacing(15);

    auto *currency =
        new QComboBox;

    currency->addItem(
        "Рубли (₽)",
        "RUB"
    );

    currency->addItem(
        "Доллары ($)",
        "USD"
    );

    currency->addItem(
        "Евро (€)",
        "EUR"
    );

    currency->setCurrentIndex(
        currency->findData(
            currencyCode()
        )
    );

    auto *distance =
        new QComboBox;

    distance->addItem(
        "Километры (км)",
        "km"
    );

    distance->addItem(
        "Мили (mi)",
        "mile"
    );

    distance->setCurrentIndex(
        distance->findData(
            distanceUnit()
        )
    );

    auto *consumption =
        new QComboBox;

    consumption->addItem(
        "Литры на 100 км",
        "l100"
    );

    consumption->addItem(
        "Мили на галлон (MPG)",
        "mpg"
    );

    consumption->setCurrentIndex(
        consumption->findData(
            consumptionUnit()
        )
    );

    form->addRow(
        "Валюта:",
        currency
    );

    form->addRow(
        "Расстояние:",
        distance
    );

    form->addRow(
        "Расход топлива:",
        consumption
    );

    layout->addWidget(box);

    auto *save =
        new QPushButton(
            "Сохранить настройки"
        );

    save->setObjectName(
        "darkBtn"
    );

    save->setFocusPolicy(
        Qt::NoFocus
    );

    layout->addWidget(
        save,
        0,
        Qt::AlignLeft
    );

    auto *info =
        new QLabel(
            "Настройки меняют только отображение данных.\n"
            "В базе данные сохраняются в исходных единицах."
        );

    info->setObjectName("sub");
    info->setWordWrap(true);

    layout->addWidget(info);

    auto *exportBox =
        new QGroupBox(
            "Экспорт данных"
        );

    auto *exportLayout =
        new QVBoxLayout(
            exportBox
        );

    auto *exportText =
        new QLabel(
            "Сохраните записи CarLog "
            "в CSV-файл."
        );

    exportText->setObjectName("sub");
    exportText->setWordWrap(true);

    exportLayout->addWidget(
        exportText
    );

    auto *exportButton =
        new QPushButton(
            "Экспортировать данные в CSV"
        );

    exportButton->setObjectName(
        "darkBtn"
    );

    exportButton->setFocusPolicy(
        Qt::NoFocus
    );

    exportLayout->addWidget(
        exportButton,
        0,
        Qt::AlignLeft
    );

    layout->addWidget(
        exportBox
    );

    auto *importButton = new QPushButton("Импортировать данные из JSON");
    importButton->setObjectName("darkBtn");
    exportLayout->addWidget(importButton,0,Qt::AlignLeft);
    connect(importButton,&QPushButton::clicked,this,[this]{ importData(); });

    auto *backupBox =
        new QGroupBox(
            "Копия данных аккаунта"
        );

    auto *backupLayout =
        new QVBoxLayout(
            backupBox
        );

    auto *backupText =
        new QLabel(
            "Сохраните данные своего аккаунта в JSON."
        );

    backupText->setObjectName(
        "sub"
    );

    backupText->setWordWrap(true);

    backupLayout->addWidget(
        backupText
    );

    auto *backupButton =
        new QPushButton(
            "Создать резервную копию"
        );

    backupButton->setObjectName(
        "darkBtn"
    );

    backupButton->setFocusPolicy(
        Qt::NoFocus
    );

    backupLayout->addWidget(
        backupButton,
        0,
        Qt::AlignLeft
    );

    layout->addWidget(
        backupBox
    );

    auto *exit = new QPushButton("Выйти из аккаунта");
    exit->setObjectName("lightBtn");
    layout->addWidget(exit,0,Qt::AlignLeft);
    connect(exit,&QPushButton::clicked,this,[this]{ logout(); });
    layout->addStretch();

    connect(
        save,
        &QPushButton::clicked,
        this,
        [this,
         currency,
         distance,
         consumption]()
        {
            QSettings s(
                "CarLog",
                "CarLog"
            );

            s.setValue(
                "currency",
                currency->currentData()
            );

            s.setValue(
                "distance",
                distance->currentData()
            );

            s.setValue(
                "consumption",
                consumption->currentData()
            );

            refreshHome();
            refreshStats();

            loadRecords(
                refuelTable,
                "Заправки"
            );

            loadRecords(
                expenseTable,
                "Расходы"
            );

            loadRecords(
                serviceTable,
                "Обслуживание"
            );

            loadRecords(
                reminderTable,
                "Напоминания"
            );

            loadCars();

            QMessageBox::information(
                this,
                "Настройки",
                "Настройки сохранены."
            );
        }
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportCsv();
        }
    );

    connect(
        backupButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            backup();
        }
    );

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(page);
    return scroll;
}


// ============================================================
// REFRESH HOME
// ============================================================

