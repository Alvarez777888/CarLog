#include "mobile.hpp"
#include "pages.hpp"
#include <QLabel>
#include <QVBoxLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

QWidget *account_page(Api &api) {
    auto *page = new QWidget;
    page->setObjectName("AccountPanel");
    page->setStyleSheet("QWidget#AccountPanel{background:transparent;} QLabel{background:transparent;}");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,0,0,0);
    auto *cards=new QHBoxLayout;
    cards->setSpacing(18);
    layout->addLayout(cards);
    auto *profile = new QGroupBox("Профиль");
    auto *profileForm = new QFormLayout(profile);
    auto *username = new QLabel;
    username->setTextFormat(Qt::PlainText);
    auto *email = new QLineEdit;
    email->setPlaceholderText("Email для восстановления пароля");
    auto *save = new QPushButton("Сохранить email");
    save->setObjectName("darkBtn");
    auto *changePassword = new QPushButton("Изменить пароль");
    changePassword->setObjectName("lightBtn");
    profileForm->addRow("Логин",username);
    profileForm->addRow("Email",email);
    profileForm->addRow(save);
    profileForm->addRow(changePassword);
    cards->addWidget(profile,1);
    auto *telegramBox = new QGroupBox("Telegram · чеки и напоминания");
    auto *telegramLayout = new QVBoxLayout(telegramBox);
    auto *info = new QLabel("Загрузка статуса…");
    info->setWordWrap(true);
    auto *code = new QLineEdit;
    code->setReadOnly(true);
    code->setPlaceholderText("Код появится после нажатия кнопки");
    auto *link = new QPushButton("Подключить Telegram");
    auto *copy = new QPushButton("Скопировать код");
    auto *open = new QPushButton("Открыть бота и привязать");
    open->hide();
    copy->hide();
    code->hide();
    auto *unlink = new QPushButton("Отвязать Telegram");
    for (auto *button : {link,copy,open,unlink}) button->setObjectName("lightBtn");
    for (QWidget *widget : QList<QWidget *>{info,code,link,open,copy,unlink}) telegramLayout->addWidget(widget);
    cards->addWidget(telegramBox,1);
    auto *notice = new QLabel;
    notice->setWordWrap(true);
    notice->setTextFormat(Qt::PlainText);
    layout->addWidget(notice);
    auto refresh=[=,&api] {
      api.call(page,"GET","/api/account",{},[=](const auto &doc,const auto &error) {
        if (!error.isEmpty()) { notice->setText(error); return; }
        username->setText(doc.object().value("login").toString());
        if (!email->isModified()) email->setText(doc.object().value("email").toString());
        unlink->setVisible(doc.object().value("telegram_linked").toBool());
        info->setText(doc.object().value("telegram_linked").toBool() ?
            "Telegram подключён. Отправляйте фото чеков в личный чат с ботом." :
            "Подключите бота, чтобы получать напоминания и добавлять заправки по фото чека.");
    });
    };
    auto *timer=new QTimer(page);
    QObject::connect(timer,&QTimer::timeout,page,[=]{if (page->isVisible()) refresh();});
    timer->start(10000);
    QTimer::singleShot(0,page,refresh);
    QObject::connect(open,&QPushButton::clicked,page,[=] {
        const auto url=QUrl(open->property("bot_url").toString());
        if (url.scheme()=="https" && url.host()=="t.me") QDesktopServices::openUrl(url);
    });
    QObject::connect(changePassword,&QPushButton::clicked,page,[=,&api] {
        auto *dialog=new QDialog(page);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("Изменить пароль");
        auto *form=new QFormLayout(dialog);
        auto *old=new QLineEdit;
        auto *next=new QLineEdit;
        auto *repeat=new QLineEdit;
        for (auto *line : {old,next,repeat}) line->setEchoMode(QLineEdit::Password);
        form->addRow("Текущий пароль",old);
        form->addRow("Новый пароль",next);
        form->addRow("Повторите пароль",repeat);
        auto *status=new QLabel;
        status->setWordWrap(true);
        status->setTextFormat(Qt::PlainText);
        auto *submit=new QPushButton("Сохранить пароль");
        submit->setObjectName("darkBtn");
        form->addRow(status);
        form->addRow(submit);
        QObject::connect(submit,&QPushButton::clicked,dialog,[=,&api] {
            if (next->text()!=repeat->text() || next->text().size()<6) {
                status->setText("Пароли должны совпадать и содержать минимум 6 символов."); return;
            }
            submit->setEnabled(false);
            api.call(dialog,"POST","/api/account/password",{{"old_password",old->text()},{"new_password",next->text()}},
                [=](const auto &,const auto &error) {
                    if (!error.isEmpty()) { status->setText(error); submit->setEnabled(true); return; }
                    dialog->accept();
                    notice->setText("Пароль изменён. На других устройствах потребуется войти заново.");
                });
        });
        mobile_dialog(dialog);
    dialog->open();
    });
    auto request = [page,&api,notice](QPushButton *button,QString path,QJsonObject body,Api::Result result) {
        button->setEnabled(false);
        notice->setText("Загрузка…");
        api.call(page,"POST",path,body,[button,notice,result](const auto &doc,const auto &error) {
            button->setEnabled(true);
            notice->setText(error);
            if (error.isEmpty()) result(doc,error);
        });
    };
    QObject::connect(link,&QPushButton::clicked,page,[=] {
        request(link,"/api/telegram/link-code",{},[code,open,copy](const auto &data,const auto &) {
            auto value = data.object().value("code").toString();
            code->setText(value);
            open->setProperty("bot_url",data.object().value("bot_url").toString());
            code->show();
            copy->show();
            open->setVisible(!open->property("bot_url").toString().isEmpty());
        });
    });
    QObject::connect(copy,&QPushButton::clicked,page,[=] { QGuiApplication::clipboard()->setText(code->text()); });
    QObject::connect(unlink,&QPushButton::clicked,page,[=] {
        if (QMessageBox::question(page,"Telegram","Отвязать бота от аккаунта?")!=QMessageBox::Yes) return;
        request(unlink,"/api/telegram/unlink",{},[=](const auto &,const auto &) {
            code->clear();
            open->setEnabled(false);
            info->setText("Telegram отключён.");
            notice->setText("Telegram отвязан.");
        });
    });
    QObject::connect(save,&QPushButton::clicked,page,[=] {
        request(save,"/api/account/email",{{"email",email->text()}},[=](const auto &,const auto &) {
            email->setModified(false);
            notice->setText("Email сохранён.");
        });
    });
    return page;
}
