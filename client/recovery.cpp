#include "mobile.hpp"
#include "loginwindow.hpp"
#include <QLabel>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTimer>

void recover(QWidget *parent,Api &api) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Восстановление пароля");
    QFormLayout form(&dialog);
    QLineEdit login,code,password;
    password.setEchoMode(QLineEdit::Password);
    QComboBox method;
    method.addItem("Telegram","telegram");
    method.addItem("Email","email");
    QPushButton send("Запросить код"), verify("Проверить код"), reset("Сохранить пароль");
    reset.setEnabled(false);
    QLabel notice;
    notice.setWordWrap(true);
    notice.setTextFormat(Qt::PlainText);
    form.addRow("Логин или email",&login);
    form.addRow("Получить через",&method);
    form.addRow(&send);
    form.addRow("Код",&code);
    form.addRow(&verify);
    form.addRow("Новый пароль",&password);
    form.addRow(&reset);
    form.addRow(&notice);
    QString token;
    auto data = [&] { return QJsonObject{{"login_or_email",login.text()},{"method",method.currentData().toString()}}; };
    QObject::connect(&send,&QPushButton::clicked,&dialog,[&] {
        send.setEnabled(false);
        api.call(&dialog,"POST","/api/recovery/request",data(),[&](const auto &,const auto &error) {
            send.setEnabled(true);
            notice.setText(error.isEmpty() ? "Если способ восстановления настроен, код придёт в течение минуты." : error);
        });
    });
    QObject::connect(&verify,&QPushButton::clicked,&dialog,[&] {
        verify.setEnabled(false);
        auto body = data();
        body.insert("code",code.text());
        api.call(&dialog,"POST","/api/recovery/verify",body,[&](const auto &doc,const auto &error) {
            verify.setEnabled(true);
            token = doc.object().value("reset_token").toString();
            reset.setEnabled(!token.isEmpty());
            notice.setText(error.isEmpty() ? "Введите новый пароль." : error);
        });
    });
    QObject::connect(&reset,&QPushButton::clicked,&dialog,[&] {
        reset.setEnabled(false);
        api.call(&dialog,"POST","/api/recovery/reset",{{"reset_token",token},{"new_password",password.text()}},
                 [&](const auto &,const auto &error) {
            if (error.isEmpty()) dialog.accept();
            else { notice.setText(error); reset.setEnabled(true); }
        });
    });
    mobile_dialog(&dialog);
    dialog.exec();
}
