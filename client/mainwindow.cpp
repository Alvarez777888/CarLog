#include "mainwindow.hpp"
#include "theme.hpp"
#include "mobile.hpp"
#include <QComboBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QTimer>
#include <QPushButton>
#include <QStyle>

QString recordKind(const QString &type) {
    if (type == "Заправки") return "fuel";
    if (type == "Расходы") return "expense";
    if (type == "Обслуживание") return "service";
    return "reminder";
}
QString json_text(QJsonValue v) {
    if (v.isString()) return v.toString();
    if (v.isDouble()) return QString::number(v.toDouble(), 'g', 15);
    return {};
}
MainWindow::MainWindow(Api &api, QString login, std::function<void()> logout)
    : api(api), login(std::move(login)), logout(std::move(logout)) {
    setWindowTitle("CarLog");
    resize(1280, 800);
    setMinimumSize(mobile_ui() ? QSize(0,0) : QSize(1050,650));
    if (mobile_ui()) resize(390,780);
    setStyleSheet(app_theme());
    auto *central = new QWidget;
    auto *layout = new QBoxLayout(mobile_ui() ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight,central);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    if (!mobile_ui()) layout->addWidget(sidebar());
    pages = new QStackedWidget;
    pages->addWidget(home());
    pages->addWidget(carsPage());
    for (const auto &type : {"Заправки","Расходы","Обслуживание","Напоминания"})
        pages->addWidget(recordsPage(type));
    pages->addWidget(statistics());
    pages->addWidget(settings());
    if (mobile_ui()) {
        auto *navigation=new QComboBox;
        navigation->setObjectName("mobileNavigation");
        navigation->addItems({"Главная","Автомобили","Заправки","Расходы","Обслуживание","Напоминания","Статистика","Настройки"});
        navigation->setMinimumHeight(48);
        layout->addWidget(navigation);
        for (int i=0;i<pages->count();++i) mobile_content(pages->widget(i));
        for (int i=0;i<pages->count();++i) {
            auto *page=pages->widget(i);
            if (qobject_cast<QScrollArea *>(page)) continue;
            pages->removeWidget(page);
            auto *scroll=new QScrollArea;
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidget(page);
            QScroller::grabGesture(scroll->viewport(),QScroller::TouchGesture);
            pages->insertWidget(i,scroll);
        }
        pages->setCurrentIndex(0);
        connect(navigation,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int index) {
            pages->setCurrentIndex(index); refreshCurrent();
        });
        connect(pages,&QStackedWidget::currentChanged,navigation,&QComboBox::setCurrentIndex);
        setStyleSheet(app_theme()+"QPushButton,QComboBox,QLineEdit,QDateEdit{min-height:36px;} QTableWidget{font-size:13px;}");
    }
    layout->addWidget(pages,1);
    setCentralWidget(central);
    notice = new QLabel;
    notice->setWordWrap(true);
    notice->setTextFormat(Qt::PlainText);
    statusBar()->addWidget(notice,1);
    statusBar()->setSizeGripEnabled(false);
    connect(pages, &QStackedWidget::currentChanged, this, [this](int index) {
        const auto buttons = findChildren<QPushButton *>("nav");
        for (int i=0;i<buttons.size();++i) {
            buttons[i]->setProperty("active", i==index);
            buttons[i]->style()->unpolish(buttons[i]);
            buttons[i]->style()->polish(buttons[i]);
        }
    });
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { refreshCurrent(); });
    timer->start(10000);
    loadCars();
}
void MainWindow::fail(QString error) { notice->setText(error); }
void MainWindow::reloadForCurrentUser() {
    loadCars();
    refreshCurrent();
}
void MainWindow::refreshCurrent() {
    switch (pages->currentIndex()) {
        case 0: loadCars(); break;
        case 1: loadCars(); break;
        case 2: loadRecords(refuelTable,"Заправки"); break;
        case 3: loadRecords(expenseTable,"Расходы"); break;
        case 4: loadRecords(serviceTable,"Обслуживание"); break;
        case 5: loadRecords(reminderTable,"Напоминания"); break;
        case 6: refreshStats(); break;
    }
}
