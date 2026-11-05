#include "mobile.hpp"
#include "mainwindow.hpp"
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QUuid>
#include <QMessageBox>
#include <QCheckBox>

static void show_entry(QWidget *parent, Api &api, const QString &kind, const QJsonObject &row,
                       const QJsonArray &cars, std::function<void()> saved) {
    auto *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    const auto title = kind=="car" ? "Автомобиль" : kind=="fuel" ? "Заправка" : kind=="expense" ? "Расход" :
                       kind=="service" ? "Обслуживание" : "Напоминание";
    dialog->setWindowTitle(title);
    dialog->setMinimumWidth(460);
    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(28,24,28,24);
    auto *heading = new QLabel((row.isEmpty() ? QString("Добавить · ") : QString("Изменить · "))+title);
    heading->setObjectName("pageTitle");
    layout->addWidget(heading);
    auto *form = new QFormLayout;
    form->setSpacing(10);
    layout->addLayout(form);
    QMap<QString,QLineEdit *> inputs;
    QMap<QString,QComboBox *> choices;
    auto add = [&](const QString &key,const QString &label,bool required=false) {
        QStringList options;
        if (key=="fuel") options = QStringList{"АИ-92","АИ-95","АИ-98","АИ-100","Дизель","Газ (пропан)","Газ (метан)","Электро","Гибрид"};
        if (key=="brand") options = QStringList{"Toyota","LADA","Kia","Hyundai","Volkswagen","Renault","Nissan","BMW","Mercedes-Benz","Skoda","Ford","Chevrolet","Honda","Mazda","Geely","Chery","Haval"};
        if (key=="category") options = kind=="service" ? QStringList{"Замена масла","Фильтры","Тормоза","Шины","Диагностика","Плановое ТО","Ремонт двигателя","Другое"} :
            QStringList{"Мойка","Парковка","Страховка","Запчасти","Штрафы","Налоги","Платные дороги","Другое"};
        if (key=="place") options = kind=="fuel" ? QStringList{"Лукойл","Газпромнефть","Роснефть","Татнефть","Тебойл","Башнефть","Другая АЗС"} : QStringList{"Автосервис","Дилер","Самостоятельно","Другое"};
        if (key=="description" && kind=="reminder") options = QStringList{"Замена масла","Продлить ОСАГО","Технический осмотр","Сезонная смена шин","Плановое ТО","Замена фильтров","Другое"};
        if (key=="description" && kind=="service") options = QStringList{"Замена масла","Замена фильтров","Замена колодок","Шиномонтаж","Диагностика","Плановое ТО","Другое"};
        if (key=="year") for (int y=QDate::currentDate().year()+1; y>=1900; --y) options.append(QString::number(y));
        if (key=="engine") options = QStringList{"0","0.8","1.0","1.2","1.4","1.5","1.6","1.8","2.0","2.4","2.5","3.0","3.5","4.0","5.0"};
        if (!options.isEmpty()) {
            auto *combo = new QComboBox;
            combo->setEditable(true);
            combo->addItems(options);
            combo->setCurrentIndex(-1);
            combo->setEditText(json_text(row.value(key)));
            combo->lineEdit()->setPlaceholderText(required ? "Выберите или введите своё" : "Выберите из списка");
            combo->lineEdit()->setMaxLength(key=="description" ? 200 : 100);
            choices.insert(key,combo);
            inputs.insert(key,combo->lineEdit());
            form->addRow(label,combo);
            return;
        }
        auto *input = new QLineEdit(json_text(row.value(key)));
        input->setMaxLength(key=="comment" ? 300 : 200);
        input->setPlaceholderText(required ? "Обязательное поле" : "Необязательно");
        inputs.insert(key,input);
        form->addRow(label,input);
    };
    QComboBox *car = nullptr;
    QDateEdit *date = nullptr;
    QDateEdit *next = nullptr;
    QCheckBox *scheduled = nullptr;
    if (kind=="car") {
        add("brand","Марка",true);
        add("model","Модель",true);
        auto *modelLine = inputs.take("model");
        form->removeRow(modelLine);
        auto *models = new QComboBox;
        models->setEditable(true);
        models->lineEdit()->setMaxLength(50);
        choices.insert("model",models);
        inputs.insert("model",models->lineEdit());
        form->insertRow(1,"Модель",models);
        const QMap<QString,QStringList> modelLists{
            {"Toyota",{"Camry","Corolla","RAV4","Land Cruiser","Yaris"}},
            {"LADA",{"Granta","Vesta","Niva","Largus","Priora"}},
            {"Kia",{"Rio","Ceed","Sportage","Sorento","K5"}},
            {"Hyundai",{"Solaris","Creta","Tucson","Elantra","Santa Fe"}},
            {"Volkswagen",{"Polo","Golf","Passat","Tiguan","Touareg"}},
            {"Renault",{"Logan","Sandero","Duster","Kaptur"}},
            {"Nissan",{"Qashqai","X-Trail","Almera","Juke"}},
            {"BMW",{"3 Series","5 Series","X3","X5"}},
            {"Skoda",{"Octavia","Rapid","Fabia","Kodiaq"}},
            {"Ford",{"Focus","Fiesta","Mondeo","Kuga"}}
        };
        auto updateModels = [models,modelLists](const QString &brand) {
            const auto text = models->currentText();
            models->clear();
            models->addItems(modelLists.value(brand));
            models->setEditText(text);
        };
        updateModels(choices["brand"]->currentText());
        models->setEditText(json_text(row.value("model")));
        QObject::connect(choices["brand"],&QComboBox::currentTextChanged,dialog,updateModels);
        add("year","Год выпуска");
        add("fuel","Топливо");
        add("engine","Двигатель, л");
        add("mileage","Пробег, км");

    } else {
        car = new QComboBox;
        date = new QDateEdit(QDate::currentDate());
        for (auto value : cars) {
            const auto c = value.toObject();
            car->addItem(c.value("brand").toString()+" "+c.value("model").toString(),json_text(c.value("id")));
        }
        if (!row.isEmpty()) {
            car->setCurrentIndex(car->findData(json_text(row.value("car"))));
            car->setEnabled(false);
        }
        form->addRow("Автомобиль",car);
        date->setDisplayFormat("dd.MM.yyyy");
        date->setCalendarPopup(true);
        auto old = QDate::fromString(row.value("day").toString(),Qt::ISODate);
        if (old.isValid()) date->setDate(old);
        form->addRow("Дата",date);
        if (kind=="fuel") {
            add("liters","Литры",true);
            add("price","Цена за литр");
            add("amount","Сумма");
            add("place","АЗС");
        } else {
            if (kind!="reminder") add("category","Категория");
            add("description",kind=="reminder" ? "Напоминание" : "Описание",kind=="reminder");
            if (kind!="reminder") {
                add("amount","Сумма");
                add("place","Место");
            }
        }
        if (kind!="reminder") {
            add("mileage","Пробег, км");
            auto suggest = [=](int index) {
                if (!row.isEmpty() || index<0 || index>=cars.size()) return;
                inputs["mileage"]->setText(json_text(cars[index].toObject().value("mileage")));
            };
            suggest(car->currentIndex());
            QObject::connect(car,QOverload<int>::of(&QComboBox::currentIndexChanged),dialog,suggest);
        }
        if (kind=="fuel") {
            auto calculate = [inputs] {
                bool lOk=false,pOk=false;
                double l=inputs["liters"]->text().replace(',','.').toDouble(&lOk);
                double p=inputs["price"]->text().replace(',','.').toDouble(&pOk);
                if (lOk && pOk && l>0 && p>0) inputs["amount"]->setText(QString::number(l*p,'f',2));
            };
            QObject::connect(inputs["liters"],&QLineEdit::textEdited,dialog,[calculate](const QString &){calculate();});
            QObject::connect(inputs["price"],&QLineEdit::textEdited,dialog,[calculate](const QString &){calculate();});
        }
        if (kind=="service") {
            next = new QDateEdit(QDate::currentDate().addMonths(3));
            next->setCalendarPopup(true);
            next->setDisplayFormat("dd.MM.yyyy");
            const auto oldNext=QDate::fromString(row.value("next_day").toString(),Qt::ISODate);
            if (oldNext.isValid()) next->setDate(oldNext);
            scheduled=new QCheckBox("Назначить");
            scheduled->setChecked(oldNext.isValid());
            next->setEnabled(oldNext.isValid());
            QObject::connect(scheduled,&QCheckBox::toggled,next,&QWidget::setEnabled);
            auto *scheduleRow=new QWidget;
            auto *scheduleLayout=new QHBoxLayout(scheduleRow);
            scheduleLayout->setContentsMargins(0,0,0,0);
            scheduleLayout->addWidget(scheduled);
            scheduleLayout->addWidget(next);
            form->addRow("Следующее ТО",scheduleRow);
        }
        add("comment","Комментарий");
    }
    auto *notice = new QLabel;
    notice->setWordWrap(true);
    notice->setTextFormat(Qt::PlainText);
    notice->setObjectName("sub");
    layout->addWidget(notice);
    auto *buttons = new QHBoxLayout;
    auto *submit = new QPushButton("Сохранить");
    auto *cancel = new QPushButton("Отмена");
    submit->setObjectName("darkBtn");
    cancel->setObjectName("lightBtn");
    buttons->addWidget(submit);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);
    QObject::connect(cancel,&QPushButton::clicked,dialog,&QDialog::reject);
    // Stable for a single creation dialog, including retries after a network timeout.
    const auto requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QObject::connect(submit,&QPushButton::clicked,dialog,[=,&api] {
        auto body = row;
        for (auto it=inputs.begin();it!=inputs.end();++it) body.insert(it.key(),it.value()->text());
        if (kind!="car") {
            body.insert("kind",kind);
            body.insert("car",car->currentData().toString());
            body.insert("day",date->date().toString(Qt::ISODate));
            if (kind=="service") body.insert("next_day",scheduled->isChecked() ? next->date().toString(Qt::ISODate) : "");
        }
        const auto id = json_text(row.value("id"));
        const auto path = (kind=="car" ? QString("/api/cars") : QString("/api/records"))+(id.isEmpty() ? "" : "/"+id);
        if (id.isEmpty()) body.insert("request_id",requestId);
        submit->setEnabled(false);
        cancel->setEnabled(false);
        notice->setText("Сохранение…");
        api.call(dialog,id.isEmpty() ? "POST" : "PUT",path,body,
                 [=](const auto &,const auto &error) {
            if (!error.isEmpty()) {
                auto message = error;
                for (auto it=inputs.begin();it!=inputs.end();++it) {
                    const auto *label = qobject_cast<QLabel *>(form->labelForField(it.value()));
                    if (!label && choices.contains(it.key()))
                        label=qobject_cast<QLabel *>(form->labelForField(choices[it.key()]));
                    if (label) message.replace("«"+it.key()+"»","«"+label->text()+"»");
                }
                message.replace("«day»","«Дата»").replace("«next_day»","«Следующее ТО»")
                       .replace("«car»","«Автомобиль»");
                notice->setText(message);
                submit->setEnabled(true);
                cancel->setEnabled(true);
                return;
            }
            dialog->accept();
            saved();
        });
    });
    mobile_dialog(dialog);
    dialog->open();
}
void edit_entry(QWidget *parent, Api &api, QString kind, QJsonObject row, std::function<void()> saved) {
    if (kind=="car") { show_entry(parent,api,kind,row,{},std::move(saved)); return; }
    api.call(parent,"GET","/api/cars",{},[=,&api](const auto &doc,const auto &error) {
        if (!error.isEmpty()) {
            auto *message = new QMessageBox(QMessageBox::Warning,"CarLog",error,QMessageBox::Ok,parent);
            message->setAttribute(Qt::WA_DeleteOnClose);
            message->open();
            return;
        }
        if (doc.array().isEmpty()) {
            auto *message = new QMessageBox(QMessageBox::Information,"CarLog","Сначала добавьте автомобиль.",QMessageBox::Ok,parent);
            message->setAttribute(Qt::WA_DeleteOnClose);
            message->open();
            return;
        }
        show_entry(parent,api,kind,row,doc.array(),saved);
    });
}
