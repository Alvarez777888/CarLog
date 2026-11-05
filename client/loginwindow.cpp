#include "loginwindow.hpp"
#include "mobile.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFrame>
#include <QFont>
#include <QCheckBox>
#include <QSettings>

LoginWindow::LoginWindow(Api &api, std::function<void(QString)> accepted, QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("CarLog");
    resize(960, 640);
    setMinimumSize(800, 500);

    setStyleSheet(
        "QWidget#LoginWindow {"
        "    background-color: #eef2f7;"
        "}"
        "QFrame#MainCard {"
        "    background-color: #ffffff;"
        "    border-radius: 24px;"
        "}"
        "QFrame#BrandPanel {"
        "    background-color: #172554;"
        "    border-radius: 24px;"
        "}"
        "QFrame#FormPanel {"
        "    background-color: #ffffff;"
        "    border-radius: 24px;"
        "}"
        "QLabel#Logo {"
        "    background-color: #2563eb;"
        "    color: #ffffff;"
        "    border-radius: 18px;"
        "    font-size: 30px;"
        "    font-weight: bold;"
        "}"
        "QLabel#BrandTitle {"
        "    background-color: transparent;"
        "    color: #ffffff;"
        "    font-size: 34px;"
        "    font-weight: bold;"
        "}"
        "QLabel#BrandText {"
        "    background-color: transparent;"
        "    color: #bfdbfe;"
        "    font-size: 15px;"
        "}"
        "QLabel#Feature {"
        "    background-color: transparent;"
        "    color: #e0f2fe;"
        "    font-size: 14px;"
        "}"
        "QLabel#FormTitle {"
        "    background-color: transparent;"
        "    color: #111827;"
        "    font-size: 28px;"
        "    font-weight: bold;"
        "}"
        "QLabel#FormSubtitle {"
        "    background-color: transparent;"
        "    color: #6b7280;"
        "    font-size: 14px;"
        "}"
        "QLabel#FieldLabel {"
        "    background-color: transparent;"
        "    color: #374151;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "}"
        "QLineEdit {"
        "    background-color: #f8fafc;"
        "    color: #111827;"
        "    border: 1px solid #d1d5db;"
        "    border-radius: 10px;"
        "    padding: 13px 14px;"
        "    font-size: 14px;"
        "}"
        "QLineEdit:hover {"
        "    border: 1px solid #94a3b8;"
        "}"
        "QLineEdit:focus {"
        "    background-color: #ffffff;"
        "    border: 2px solid #2563eb;"
        "}"
        "QPushButton#LoginButton {"
        "    background-color: #2563eb;"
        "    color: #ffffff;"
        "    border: none;"
        "    border-radius: 10px;"
        "    padding: 13px;"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "}"
        "QPushButton#LoginButton:hover {"
        "    background-color: #1d4ed8;"
        "}"
        "QPushButton#RegisterButton {"
        "    background-color: #eff6ff;"
        "    color: #2563eb;"
        "    border: 1px solid #bfdbfe;"
        "    border-radius: 10px;"
        "    padding: 11px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton#RegisterButton:hover {"
        "    background-color: #dbeafe;"
        "}"
        "QCheckBox#RememberMe {"
        "    background-color: transparent;"
        "    color: #4b5563;"
        "    font-size: 13px;"
        "}"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #94a3b8; border-radius: 3px; background: white; }"
        "QCheckBox::indicator:checked { background: #2563eb; border-color: #2563eb; }"
        "QLabel#BottomText {"
        "    background-color: transparent;"
        "    color: #9ca3af;"
        "    font-size: 12px;"
        "}"
    );

    setObjectName("LoginWindow");

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(45, 40, 45, 40);
    outerLayout->setAlignment(Qt::AlignCenter);

    auto *mainCard = new QFrame;
    mainCard->setObjectName("MainCard");
    mainCard->setMaximumSize(900, 560);
    mainCard->setMinimumSize(760, 490);

    auto *cardLayout = new QHBoxLayout(mainCard);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // -------------------------------------------------
    // Левая часть
    // -------------------------------------------------

    auto *brandPanel = new QFrame;
    brandPanel->setObjectName("BrandPanel");
    brandPanel->setMinimumWidth(340);
    brandPanel->setMaximumWidth(370);

    auto *brandLayout = new QVBoxLayout(brandPanel);
    brandLayout->setContentsMargins(40, 40, 40, 40);
    brandLayout->setSpacing(15);

    auto *logo = new QLabel("CL");
    logo->setObjectName("Logo");
    logo->setFixedSize(72, 72);
    logo->setAlignment(Qt::AlignCenter);

    auto *brandTitle = new QLabel("CarLog");
    brandTitle->setObjectName("BrandTitle");

    auto *brandText = new QLabel(
        "Управляйте расходами авто\n"
        "удобно и без лишних записей."
    );
    brandText->setObjectName("BrandText");
    brandText->setWordWrap(true);

    auto *feature1 =
        new QLabel("✓  Учёт расходов и топлива");
    feature1->setObjectName("Feature");

    auto *feature2 =
        new QLabel("✓  Обслуживание автомобиля");
    feature2->setObjectName("Feature");

    auto *feature3 =
        new QLabel("✓  Напоминания и статистика");
    feature3->setObjectName("Feature");

    auto *feature4 =
        new QLabel("✓  Все данные в одном месте");
    feature4->setObjectName("Feature");

    brandLayout->addWidget(logo);
    brandLayout->addSpacing(8);
    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandText);
    brandLayout->addSpacing(18);
    brandLayout->addWidget(feature1);
    brandLayout->addWidget(feature2);
    brandLayout->addWidget(feature3);
    brandLayout->addWidget(feature4);
    brandLayout->addStretch();

    // -------------------------------------------------
    // Правая часть
    // -------------------------------------------------

    auto *formPanel = new QFrame;
    formPanel->setObjectName("FormPanel");

    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(42, 28, 42, 28);
    formLayout->setSpacing(5);

    auto *formTitle =
        new QLabel("Добро пожаловать!");
    formTitle->setObjectName("FormTitle");

    auto *formSubtitle =
        new QLabel("Войдите в свой аккаунт CarLog");
    formSubtitle->setObjectName("FormSubtitle");

    auto *loginLabel = new QLabel("Логин");
    loginLabel->setObjectName("FieldLabel");

    login = new QLineEdit;
    login->setPlaceholderText("Введите логин");
    login->setFixedHeight(42);

    auto *passwordLabel = new QLabel("Пароль");
    passwordLabel->setObjectName("FieldLabel");

    password = new QLineEdit;
    password->setPlaceholderText("Введите пароль");
    password->setEchoMode(QLineEdit::Password);
    password->setFixedHeight(42);

    auto *rememberMe =
        new QCheckBox("Запомнить логин");
    rememberMe->setObjectName("RememberMe");
    rememberMe->setFixedHeight(18);
    rememberMe->setCursor(Qt::PointingHandCursor);

    // -------------------------------------------------
    // Загрузка сохранённых данных
    // -------------------------------------------------

    QSettings savedSettings(
        "CarLog",
        "CarLog"
    );

    const bool remember =
        savedSettings
            .value(
                "login/rememberMe",
                false
            )
            .toBool();

    if (remember)
    {
        login->setText(
            savedSettings
                .value(
                    "login/savedLogin",
                    ""
                )
                .toString()
        );


    }

    savedSettings.remove("login/savedPassword");
    rememberMe->setChecked(remember);

    auto *rememberRow = new QWidget;
    rememberRow->setFixedHeight(18);

    auto *rememberLayout =
        new QHBoxLayout(rememberRow);

    rememberLayout->setContentsMargins(
        0, 0, 0, 0
    );

    rememberLayout->setSpacing(0);
    rememberLayout->addWidget(rememberMe);
    rememberLayout->addStretch();

    auto *loginButton =
        new QPushButton("Войти");

    loginButton->setObjectName(
        "LoginButton"
    );

    loginButton->setFixedHeight(42);
    loginButton->setCursor(
        Qt::PointingHandCursor
    );

    auto *registerButton =
        new QPushButton("Создать аккаунт");

    registerButton->setObjectName(
        "RegisterButton"
    );

    registerButton->setFixedHeight(40);
    registerButton->setCursor(
        Qt::PointingHandCursor
    );

    auto *bottomText =
        new QLabel(
            "Нет аккаунта? Нажмите "
            "«Создать аккаунт»"
        );

    bottomText->setObjectName(
        "BottomText"
    );

    bottomText->setAlignment(
        Qt::AlignCenter
    );

    formLayout->addWidget(formTitle);
    formLayout->addWidget(formSubtitle);
    formLayout->addSpacing(12);

    formLayout->addWidget(loginLabel);
    formLayout->addWidget(login);

    formLayout->addSpacing(4);

    formLayout->addWidget(passwordLabel);
    formLayout->addWidget(password);

    formLayout->addWidget(rememberRow);
    formLayout->addSpacing(4);

    formLayout->addWidget(loginButton);
    formLayout->addSpacing(2);
    formLayout->addWidget(registerButton);

    auto *notice = new QLabel;
    notice->setObjectName("FormSubtitle");
    notice->setWordWrap(true);
    notice->setTextFormat(Qt::PlainText);
    formLayout->addWidget(notice);
    auto *recovery = new QPushButton("Забыли пароль?");
    recovery->setObjectName("RegisterButton");
    formLayout->addWidget(recovery);
    formLayout->addStretch();
    formLayout->addWidget(bottomText);

    cardLayout->addWidget(brandPanel);
    cardLayout->addWidget(formPanel);

    if (mobile_ui()) {
        setMinimumSize(0,0);
        resize(390,780);
        brandPanel->hide();
        mainCard->setMinimumSize(0,520);
        mainCard->setMaximumSize(10000,10000);
        formLayout->setContentsMargins(18,24,18,24);
        outerLayout->setContentsMargins(10,10,10,10);
        outerLayout->setAlignment(Qt::Alignment());
        formTitle->setStyleSheet("color:#18202f;background:transparent;font-size:23px;font-weight:700;");
        formTitle->setWordWrap(true);
        bottomText->setWordWrap(true);
        auto *scroll=new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(mainCard);
        outerLayout->addWidget(scroll);
    } else outerLayout->addWidget(mainCard);

    auto authenticate = [this, &api, accepted, rememberMe, notice, loginButton, registerButton, recovery] {
        const auto user = login->text().trimmed();
        const auto pass = password->text();
        if (user.isEmpty() || pass.isEmpty()) {
            notice->setText("Введите логин и пароль.");
            return;
        }
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
        recovery->setEnabled(false);
        notice->setText("Входим…");
        api.call(this, "POST", "/api/login", {{"login", user}, {"password", pass}},
            [=, &api](const auto &doc, const auto &error) {
                loginButton->setEnabled(true);
                registerButton->setEnabled(true);
                recovery->setEnabled(true);
                if (!error.isEmpty()) { notice->setText(error); return; }
                api.token = doc.object().value("token").toString();
                if (api.token.isEmpty()) { notice->setText("Не удалось войти. Повторите попытку."); return; }
                QSettings settings("CarLog", "CarLog");
                settings.setValue("login/rememberMe", rememberMe->isChecked());
                if (rememberMe->isChecked()) settings.setValue("login/savedLogin", user);
                else settings.remove("login/savedLogin");
                password->clear();
                accepted(doc.object().value("login").toString());
            });
    };
    connect(loginButton, &QPushButton::clicked, this, authenticate);
    connect(password, &QLineEdit::returnPressed, this, authenticate);
    connect(recovery, &QPushButton::clicked, this, [this, &api] { recover(this, api); });
    connect(registerButton, &QPushButton::clicked, this, [this, &api, accepted, notice] {
        auto *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("Создать аккаунт CarLog");
        dialog->setMinimumWidth(430);
        auto *form = new QFormLayout(dialog);
        form->setContentsMargins(28, 24, 28, 24);
        auto *title = new QLabel("Создать аккаунт");
        title->setObjectName("FormTitle");
        form->addRow(title);
        auto *user = new QLineEdit(login->text().trimmed());
        auto *email = new QLineEdit;
        auto *pass = new QLineEdit;
        auto *confirm = new QLineEdit;
        pass->setEchoMode(QLineEdit::Password);
        confirm->setEchoMode(QLineEdit::Password);
        email->setPlaceholderText("Для восстановления пароля");
        pass->setPlaceholderText("Минимум 6 символов");
        form->addRow("Логин", user);
        form->addRow("Email (необязательно)", email);
        form->addRow("Пароль", pass);
        form->addRow("Повторите пароль", confirm);
        auto *message = new QLabel;
        message->setWordWrap(true);
        message->setTextFormat(Qt::PlainText);
        message->setObjectName("FormSubtitle");
        form->addRow(message);
        auto *submit = new QPushButton("Создать аккаунт");
        submit->setObjectName("LoginButton");
        form->addRow(submit);
        connect(submit, &QPushButton::clicked, dialog, [=, &api] {
            const auto name = user->text().trimmed(), pw = pass->text();
            if (name.size() < 3 || pw.size() < 6) {
                message->setText("Логин — минимум 3 символа, пароль — минимум 6.");
                return;
            }
            if (pw != confirm->text()) { message->setText("Пароли не совпадают."); return; }
            submit->setEnabled(false);
            user->setEnabled(false);
            email->setEnabled(false);
            pass->setEnabled(false);
            confirm->setEnabled(false);
            message->setText("Создаём аккаунт…");
            api.call(dialog, "POST", "/api/register", {{"login", name}, {"password", pw}, {"email", email->text()}},
                [=, &api](const auto &, const auto &error) {
                    if (!error.isEmpty()) {
                        message->setText(error);
                        submit->setEnabled(true);
                        user->setEnabled(true);
                        email->setEnabled(true);
                        pass->setEnabled(true);
                        confirm->setEnabled(true);
                        return;
                    }
                    // The account is already created even if this subsequent login fails.
                    login->setText(name);
                    api.call(dialog, "POST", "/api/login", {{"login", name}, {"password", pw}},
                        [=, &api](const auto &doc, const auto &loginError) {
                            const auto token = doc.object().value("token").toString();
                            if (!loginError.isEmpty() || token.isEmpty()) {
                                notice->setText("Аккаунт создан. Введите пароль и нажмите «Войти».");
                                dialog->accept();
                                return;
                            }
                            api.token = token;
                            dialog->accept();
                            accepted(name);
                        });
                });
        });
        mobile_dialog(dialog);
        dialog->open();
    });
}
