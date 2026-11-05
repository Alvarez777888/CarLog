#pragma once
#include "api.hpp"
#include <QMainWindow>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QPointer>
#include <QTableWidget>
#include <functional>

class QStackedWidget;
class QGridLayout;
class QLabel;
class QPushButton;
class MainWindow : public QMainWindow {
public:
    MainWindow(Api &api, QString login, std::function<void()> logout);
private:
    Api &api;
    QString login;
    std::function<void()> logout;
    QStackedWidget *pages = nullptr;
    QWidget *carCards = nullptr;
    QGridLayout *carCardsLayout = nullptr;
    qint64 selectedCarId = -1;
    QTableWidget *refuelTable = nullptr, *expenseTable = nullptr, *serviceTable = nullptr, *reminderTable = nullptr;
    QLabel *homeMileage = nullptr, *homeFuel = nullptr, *homeExpenses = nullptr, *homeRecords = nullptr;
    QLabel *statsTotal = nullptr, *statsFuel = nullptr, *statsService = nullptr, *statsOther = nullptr;
    QLabel *notice = nullptr;
    QJsonArray cars;
    QMap<QString,QJsonArray> records;
    QSet<QString> loading;
    quint64 homeRequest = 0;
    QWidget *sidebar();
    QPushButton *navButton(const QString &text);
    QWidget *home();
    QWidget *carsPage();
    QWidget *recordsPage(const QString &type);
    QWidget *statistics();
    QWidget *settings();
    void refreshHome();
    void refreshStats();
    void loadCars();
    void renderCars();
    void loadRecords(QTableWidget *,const QString &type);
    void reloadForCurrentUser();
    void refreshCurrent();
    void addCar();
    void editCar();
    void deleteCar();
    void addRecord(const QString &type);
    void editRecord(const QString &type);
    void deleteRecord(const QString &type);
    void toggleReminder();
    void mutate(QString path, QByteArray method);
    void fail(QString error);
    void exportCsv();
    void backup();
    void importData();
    QJsonObject selectedRecord(const QString &type) const;
};
QString recordKind(const QString &type);
QString json_text(QJsonValue value);
void edit_entry(QWidget *parent, Api &api, QString kind, QJsonObject row, std::function<void()> saved);
