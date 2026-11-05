#include "mainwindow.hpp"
#include "formatting.hpp"
#include <QComboBox>
#include <QLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QStackedWidget>

void MainWindow::loadCars() {
    if (loading.contains("cars")) return;
    loading.insert("cars");
    api.call(this, "GET", "/api/cars", {}, [this](const auto &doc, const auto &error) {
        loading.remove("cars");
        if (!error.isEmpty()) { fail(error); return; }
        if (!doc.isArray()) { fail("Не удалось загрузить автомобили."); return; }
        if (cars != doc.array() || !carCards->layout()->count()) {
            cars = doc.array();
            renderCars();
        }
        if (pages->currentIndex()==0) refreshHome();
        notice->clear();
    });
}
void MainWindow::refreshHome() {
    const auto request = ++homeRequest;
    if (selectedCarId < 0) {
        homeMileage->setText("Пробег\n—");
        homeFuel->setText("Расход топлива\n—");
        homeExpenses->setText("Расходы\n—");
        homeRecords->setText("Записей\n—");
        return;
    }
    const auto id = selectedCarId;
    api.call(this, "GET", "/api/stats/summary?car="+QString::number(id), {},
        [this,id,request](const auto &doc, const auto &error) {
            if (selectedCarId != id || request != homeRequest) return;
            if (!error.isEmpty()) { fail(error); return; }
            const auto d = doc.object();
            homeMileage->setText("Пробег\n"+distanceText(d.value("mileage").toDouble()));
            homeMileage->setToolTip(d.value("name").toString());
            homeFuel->setText("Расход топлива\n"+consumptionText(d.value("consumption").toDouble()));
            homeExpenses->setText("Расходы\n"+money(d.value("total_expenses").toDouble()));
            homeRecords->setText("Записей\n"+json_text(d.value("records_count")));
        });
}
void MainWindow::refreshStats() {
    if (loading.contains("stats")) return;
    loading.insert("stats");
    api.call(this, "GET", "/api/stats/totals", {}, [this](const auto &doc, const auto &error) {
        loading.remove("stats");
        if (!error.isEmpty()) { fail(error); return; }
        const auto d = doc.object();
        statsTotal->setText("Всего\n"+money(d.value("total").toDouble()));
        statsFuel->setText("Заправки\n"+money(d.value("fuel").toDouble()));
        statsService->setText("Обслуживание\n"+money(d.value("service").toDouble()));
        statsOther->setText("Другие расходы\n"+money(d.value("other").toDouble()));
        notice->clear();
    });
}
void MainWindow::loadRecords(QTableWidget *table, const QString &type) {
    const auto kind = recordKind(type);
    if (loading.contains(kind)) return;
    loading.insert(kind);
    // Fetch names together with the journal so a newly added car on another device has its name.
    api.call(this, "GET", "/api/cars", {}, [this,table,type,kind](const auto &carDoc, const auto &carError) {
        if (!carError.isEmpty() || !carDoc.isArray()) {
            loading.remove(kind);
            fail(carError.isEmpty() ? "Не удалось загрузить автомобили." : carError);
            return;
        }
        const auto names = carDoc.array();
        api.call(this, "GET", "/api/records?kind="+kind, {}, [this,table,type,kind,names](const auto &doc,const auto &error) {
            loading.remove(kind);
            if (!error.isEmpty()) { fail(error); return; }
            if (!doc.isArray()) { fail("Не удалось загрузить журнал."); return; }
            QVariant selected;
            if (table->currentRow()>=0 && table->item(table->currentRow(),0))
                selected = table->item(table->currentRow(),0)->data(Qt::UserRole);
            records[kind] = doc.array();
            table->setRowCount(0);
            for (const auto &value : doc.array()) {
                const auto row = value.toObject();
                QString name;
                for (const auto &entry : names) {
                    const auto car = entry.toObject();
                    if (car.value("id") == row.value("car"))
                        name = car.value("brand").toString()+" "+car.value("model").toString();
                }
                const auto date = displayDate(row.value("day").toString());
                const auto amount = money(row.value("amount").toDouble());
                const auto km = distanceText(row.value("mileage").toDouble());
                QStringList cells;
                if (kind=="fuel") cells = QStringList{date,name,QString::number(row.value("liters").toDouble(),'f',2),
                    money(row.value("price").toDouble()),amount,km,row.value("place").toString(),row.value("comment").toString()};
                else if (kind=="expense") cells = QStringList{date,row.value("category").toString(),row.value("description").toString(),
                    amount,km,name,row.value("comment").toString()};
                else if (kind=="service") cells = QStringList{date,row.value("description").toString(),km,amount,
                    row.value("place").toString(),name,row.value("comment").toString()};
                else cells = QStringList{date,row.value("description").toString(),name,row.value("done").toInt() ? "Выполнено" : "Ожидает"};
                const auto index = table->rowCount();
                table->insertRow(index);
                for (int j=0;j<cells.size();++j) table->setItem(index,j,new QTableWidgetItem(cells[j]));
                const auto id = static_cast<qint64>(row.value("id").toDouble());
                table->item(index,0)->setData(Qt::UserRole,QVariant::fromValue(id));
                if (selected.isValid() && selected.toLongLong()==id) table->selectRow(index);
            }
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
            table->horizontalHeader()->setStretchLastSection(true);
            notice->clear();
        });
    });
}
void MainWindow::addCar() { edit_entry(this,api,"car",{},[this]{ reloadForCurrentUser(); }); }
void MainWindow::editCar() {
    for (auto value : cars)
        if (static_cast<qint64>(value.toObject().value("id").toDouble())==selectedCarId) {
            edit_entry(this,api,"car",value.toObject(),[this]{ reloadForCurrentUser(); });
            return;
        }
    fail("Выберите автомобиль.");
}
void MainWindow::mutate(QString path, QByteArray method) {
    if (loading.contains(path)) return;
    loading.insert(path);
    api.call(this,method,path,{},[this,path](const auto &,const auto &error) {
        loading.remove(path);
        if (!error.isEmpty()) { fail(error); return; }
        reloadForCurrentUser();
    });
}
void MainWindow::deleteCar() {
    if (selectedCarId < 0) { fail("Выберите автомобиль."); return; }
    if (QMessageBox::question(this,"Удалить автомобиль","Удалить автомобиль и все его записи?") == QMessageBox::Yes)
        mutate("/api/cars/"+QString::number(selectedCarId),"DELETE");
}
QJsonObject MainWindow::selectedRecord(const QString &type) const {
    QTableWidget *table = type=="Заправки" ? refuelTable : type=="Расходы" ? expenseTable :
                         type=="Обслуживание" ? serviceTable : reminderTable;
    if (table->currentRow()<0 || !table->item(table->currentRow(),0)) return {};
    const auto id = table->item(table->currentRow(),0)->data(Qt::UserRole).toLongLong();
    for (auto value : records.value(recordKind(type)))
        if (static_cast<qint64>(value.toObject().value("id").toDouble())==id) return value.toObject();
    return {};
}
void MainWindow::addRecord(const QString &type) {
    edit_entry(this,api,recordKind(type),{},[this]{ reloadForCurrentUser(); });
}
void MainWindow::editRecord(const QString &type) {
    const auto row = selectedRecord(type);
    if (row.isEmpty()) { fail("Выберите запись."); return; }
    edit_entry(this,api,recordKind(type),row,[this]{ reloadForCurrentUser(); });
}
void MainWindow::deleteRecord(const QString &type) {
    const auto row = selectedRecord(type);
    if (row.isEmpty()) { fail("Выберите запись."); return; }
    if (QMessageBox::question(this,"Удалить запись","Удалить выбранную запись?")==QMessageBox::Yes)
        mutate("/api/records/"+json_text(row.value("id")),"DELETE");
}
void MainWindow::toggleReminder() {
    const auto row = selectedRecord("Напоминания");
    if (row.isEmpty()) { fail("Выберите напоминание."); return; }
    mutate("/api/records/"+json_text(row.value("id"))+"/toggle","POST");
}
