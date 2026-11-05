#include "mainwindow.hpp"
#include <QFileDialog>
#include <QSaveFile>
#include <QMessageBox>
#include <QLabel>

static QString csv_cell(QString s) {
    if (!s.isEmpty() && QString("=+-@\t\r").contains(s.front())) s.prepend('\'');
    s.replace('"', "\"\"");
    return '"'+s+'"';
}
void MainWindow::exportCsv() {
    const auto path = QFileDialog::getSaveFileName(this,"Экспорт записей","carlog.csv","CSV (*.csv)");
    if (path.isEmpty()) return;
    api.call(this,"GET","/api/account/export",{},[this,path](const auto &doc,const auto &error) {
        if (!error.isEmpty()) { fail(error); return; }
        const QStringList keys{"day","car","kind","category","description","amount","mileage","liters","price","place","comment","next_day","done"};
        QString output = keys.join(';')+"\r\n";
        for (const auto &value : doc.object().value("records").toArray()) {
            const auto row = value.toObject();
            QStringList cells;
            for (const auto &key : keys) cells.append(csv_cell(json_text(row.value(key))));
            output += cells.join(';')+"\r\n";
        }
        QSaveFile file(path);
        const auto bytes = QByteArray("\xef\xbb\xbf")+output.toUtf8();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
            fail("Не удалось сохранить файл."); return;
        }
        notice->setText("Записи сохранены.");
    });
}
void MainWindow::backup() {
    const auto path = QFileDialog::getSaveFileName(this,"Копия данных аккаунта","carlog-account.json","JSON (*.json)");
    if (path.isEmpty()) return;
    api.call(this,"GET","/api/account/export",{},[this,path](const auto &doc,const auto &error) {
        if (!error.isEmpty()) { fail(error); return; }
        QSaveFile file(path);
        const auto bytes = doc.toJson();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
            fail("Не удалось сохранить файл."); return;
        }
        notice->setText("Данные аккаунта сохранены.");
    });
}

void MainWindow::importData() {
    const auto path = QFileDialog::getOpenFileName(this,"Импорт данных CarLog",{},"Копия CarLog (*.json)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size()>900000) { fail("Файл недоступен или превышает 900 КБ."); return; }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject() || doc.object().value("format").toInt()!=1 ||
        !doc.object().value("cars").isArray() || !doc.object().value("records").isArray()) {
        fail("Выберите JSON, сохранённый через «Копия данных аккаунта»."); return;
    }
    if (QMessageBox::question(this,"Импорт",
        QString("Добавить автомобили (%1) и записи (%2) в ваш аккаунт? Существующие данные сохранятся.")
        .arg(doc.object().value("cars").toArray().size()).arg(doc.object().value("records").toArray().size()))!=QMessageBox::Yes) return;
    api.call(this,"POST","/api/account/import",doc.object(),[this](const auto &result,const auto &error) {
        if (!error.isEmpty()) { fail(error); return; }
        reloadForCurrentUser();
        QMessageBox::information(this,"Импорт",result.object().value("repeated").toBool() ?
            "Этот файл уже был импортирован. Дубли не созданы." : "Данные импортированы.");
    });
}
