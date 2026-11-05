#include "api.hpp"
#include "mobile.hpp"
#if __has_include("mobile-config.hpp")
#include "mobile-config.hpp"
#endif
#include "loginwindow.hpp"
#include "mainwindow.hpp"
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSettings>
#include <QUrl>
#include <QPalette>

int main(int argc, char **argv) {
    QApplication app(argc,argv);
    app.setStyle("Fusion");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#eef2f7"));
    palette.setColor(QPalette::WindowText, QColor("#18202f"));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor("#f8fafc"));
    palette.setColor(QPalette::Text, QColor("#18202f"));
    palette.setColor(QPalette::Button, Qt::white);
    palette.setColor(QPalette::ButtonText, QColor("#18202f"));
    palette.setColor(QPalette::Highlight, QColor("#2563eb"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);
    app.setOrganizationName("CarLog");
    app.setApplicationName("CarLog");
    app.setWindowIcon(QIcon(":/resources/CarLog.png"));
    Api api;
    // Configured by the application publisher. End users only see sign-in and registration.
    QSettings config(QDir(QCoreApplication::applicationDirPath()).filePath("carlog.ini"),QSettings::IniFormat);
    auto server = qEnvironmentVariable("CARLOG_SERVER_URL");
#ifdef CARLOG_API_URL
    if (server.isEmpty()) server=QString::fromUtf8(CARLOG_API_URL);
#endif
    if (server.isEmpty()) server = config.value("server/url","http://127.0.0.1:8000").toString();
    const auto url = QUrl(server.trimmed(),QUrl::StrictMode);
    const bool local = url.host()=="localhost" || url.host()=="127.0.0.1" || url.host()=="::1";
    const bool valid = url.isValid() && !url.host().isEmpty() && !url.hasQuery() && !url.hasFragment() &&
                       url.userInfo().isEmpty() && (url.path().isEmpty() || url.path()=="/") &&
                       (url.scheme()=="https" || (!mobile_ui() && local && url.scheme()=="http"));
    api.base = valid ? url.adjusted(QUrl::StripTrailingSlash).toString() : QString();
    QWidget *window = nullptr;
    std::function<void()> showLogin;
    showLogin = [&] {
        auto *login = new LoginWindow(api,[&](QString name) {
            auto *next = new MainWindow(api,name,[&] {
                api.call(&api,"POST","/api/logout",{},[](const auto &,const auto &) {});
                api.token.clear();
                showLogin();
            });
            auto *old = window;
            window = next;
            next->show();
            if (old) { old->hide(); old->deleteLater(); }
        });
        auto *old = window;
        window = login;
        login->show();
        if (old) { old->hide(); old->deleteLater(); }
    };
    showLogin();
    const auto result = app.exec();
    delete window;
    return result;
}
