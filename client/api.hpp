#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

class Api : public QObject {
public:
    using Result = std::function<void(QJsonDocument, QString)>;
    QString base, token;
    explicit Api(QObject *parent = nullptr) : QObject(parent) {}
    void call(QObject *owner, QByteArray method, QString path, QJsonObject body, Result callback);
private:
    QNetworkAccessManager network;
};
