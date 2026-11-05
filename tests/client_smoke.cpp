// Run against a disposable CarLog server. Set CARLOG_TEST_URL and optionally CARLOG_QA_DIR.
#include "api.hpp"
#include "mobile.hpp"
#include "loginwindow.hpp"
#include "mainwindow.hpp"
#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QToolButton>
#include <QComboBox>
#include <iostream>
#include <stdexcept>

static void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
static void wait_for(std::function<bool()> done) {
    QElapsedTimer timer;
    timer.start();
    while (!done() && timer.elapsed()<15000) {
        QApplication::processEvents();
        QThread::msleep(10);
    }
    check(done(),"Timed out waiting for application state");
}
static QPushButton *button(QWidget *widget, QString text) {
    for (auto *b : widget->findChildren<QPushButton *>()) if (b->text()==text && b->isVisibleTo(widget)) return b;
    if (auto *navigation=widget->findChild<QComboBox *>("mobileNavigation")) {
        auto index=navigation->findText(text);
        if (index>=0) {
            navigation->setCurrentIndex(index);
            auto *proxy=new QPushButton(widget);
            proxy->hide();
            return proxy;
        }
    }
    throw std::runtime_error("Button not found: "+text.toStdString());
}
static QDialog *visible_dialog(QWidget *parent) {
    for (auto *dialog : parent->findChildren<QDialog *>()) if (dialog->isVisible()) return dialog;
    return nullptr;
}
static QLineEdit *field(QDialog *dialog, QString label) {
    for (auto *form : dialog->findChildren<QFormLayout *>()) {
        for (auto *line : dialog->findChildren<QLineEdit *>()) {
            auto *text = qobject_cast<QLabel *>(form->labelForField(line));
            if (!text) if (auto *combo=qobject_cast<QComboBox *>(line->parentWidget()))
                text=qobject_cast<QLabel *>(form->labelForField(combo));
            if (text && text->text()==label) return line;
        }
    }
    throw std::runtime_error("Field not found: "+label.toStdString());
}
static void screenshot(QWidget &widget, const char *name) {
    auto dir = qEnvironmentVariable("CARLOG_QA_DIR");
    if (dir.isEmpty()) return;
    QDir().mkpath(dir);
    QApplication::processEvents();
    check(widget.grab().save(QDir(dir).filePath(QString::fromLatin1(name)+".png")),"Screenshot failed");
}
int main(int argc,char **argv) try {
    QApplication app(argc,argv);
    app.setStyle("Fusion");
    app.setOrganizationName("CarLogTests");
    app.setApplicationName("Smoke");
    Api api;
    api.base = qEnvironmentVariable("CARLOG_TEST_URL","http://127.0.0.1:8000");
    QString accepted;
    LoginWindow login(api,[&](QString name){ accepted=name; });
    login.show();
    check(login.findChildren<QLineEdit *>().size()==2,"Login exposes extra fields");
    screenshot(login,"login");
    button(&login,"Создать аккаунт")->click();
    auto *dialog = login.findChild<QDialog *>();
    check(dialog && dialog->isVisible(),"Registration dialog is missing");
    const auto username = "ui_"+QString::number(QDateTime::currentMSecsSinceEpoch());
    field(dialog,"Логин")->setText(username);
    field(dialog,"Пароль")->setText("test-secret123");
    field(dialog,"Повторите пароль")->setText("test-secret123");
    screenshot(*dialog,"registration");
    button(dialog,"Создать аккаунт")->click();
    wait_for([&]{return !accepted.isEmpty();});
    check(accepted==username && !api.token.isEmpty(),"Registration did not enter this account");
    bool fetched=false;
    api.call(&api,"GET","/api/cars",{},[&](const auto &doc,const auto &error) {
        check(error.isEmpty() && doc.isArray() && doc.array().isEmpty(),"New account has unexpected cars");
        fetched=true;
    });
    wait_for([&]{return fetched;});
    login.hide();
    MainWindow main(api,username,[]{});
    main.show();
    screenshot(main,"home-empty");
    button(&main,"Автомобили")->click();
    button(&main,"Добавить автомобиль")->click();
    auto *edit = visible_dialog(&main);
    check(edit && edit->isVisible(),"Car dialog is missing");
    field(edit,"Марка")->setText("Toyota");
    button(edit,"Сохранить")->click();
    wait_for([&]{return button(edit,"Сохранить")->isEnabled();});
    check(edit->isVisible() && field(edit,"Марка")->text()=="Toyota","Failed form lost user input");
    field(edit,"Модель")->setText("Camry");
    field(edit,"Год выпуска")->setText("2019");
    field(edit,"Пробег, км")->setText("1000");
    check(!mobile_ui() || edit->width()<=main.width(),"Mobile form exceeds phone width");
    screenshot(*edit,"car-form");
    button(edit,"Сохранить")->click();
    QPointer<QDialog> pending = edit;
    wait_for([&]{return !pending || !pending->isVisible();});
    bool exists=false;
    api.call(&api,"GET","/api/cars",{},[&](const auto &doc,const auto &error) {
        exists=error.isEmpty() && doc.array().size()==1 && doc.array()[0].toObject().value("brand")=="Toyota";
    });
    wait_for([&]{return exists;});
    wait_for([&] { return !main.findChildren<QToolButton *>().isEmpty(); });
    screenshot(main,"cars");
    button(&main,"Заправки")->click();
    button(&main,"Добавить")->click();
    wait_for([&]{return visible_dialog(&main)!=nullptr;});
    auto *fuel = visible_dialog(&main);
    field(fuel,"Литры")->setText("40");
    field(fuel,"Цена за литр")->setText("60");
    screenshot(*fuel,"fuel-form");
    button(fuel,"Сохранить")->click();
    pending=fuel;
    wait_for([&]{return !pending || !pending->isVisible();});
    bool saved=false;
    api.call(&api,"GET","/api/records?kind=fuel",{},[&](const auto &doc,const auto &error) {
        saved=error.isEmpty() && doc.array().size()==1 && doc.array()[0].toObject().value("amount").toDouble()==2400;
    });
    wait_for([&]{return saved;});
    button(&main,"Настройки")->click();
    screenshot(main,"settings");
    std::cout << "OK: original login, registration, empty account, car and fuel creation through desktop UI\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
