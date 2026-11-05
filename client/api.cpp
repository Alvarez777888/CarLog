#include "api.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

void Api::call(QObject *owner, QByteArray method, QString path, QJsonObject body, Result callback) {
    if (base.isEmpty()) {
        QTimer::singleShot(0,owner,[callback] { callback({}, "Сервис временно недоступен. Попробуйте позже."); });
        return;
    }
    QNetworkRequest req(QUrl(base + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    if (!token.isEmpty()) req.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    auto *reply = network.sendCustomRequest(req,method,
        method == "GET" || method == "DELETE" ? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact));
    QTimer::singleShot(12000,reply,[reply] { if (!reply->isFinished()) reply->abort(); });
    QObject::connect(reply,&QNetworkReply::finished,reply,&QObject::deleteLater);
    QObject::connect(reply,&QNetworkReply::finished,owner,[reply,callback=std::move(callback)] {
        const auto data = QJsonDocument::fromJson(reply->readAll());
        auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error;
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            error = data.object().value("error").toString();
            if (error.isEmpty()) error = "Не удалось связаться с CarLog. Проверьте интернет и повторите попытку.";
        } else if (data.isNull()) {
            error = "Сервер вернул неожиданный ответ.";
        }
        callback(data,error);
    });
}
