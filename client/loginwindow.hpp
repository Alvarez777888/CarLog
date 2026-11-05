#pragma once
#include "api.hpp"
#include <QWidget>
#include <functional>

class QLineEdit;
class LoginWindow : public QWidget {
public:
    LoginWindow(Api &api, std::function<void(QString)> accepted, QWidget *parent = nullptr);
private:
    QLineEdit *login = nullptr;
    QLineEdit *password = nullptr;
};
void recover(QWidget *parent, Api &api);
