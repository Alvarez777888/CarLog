#include "core.hpp"

static Params values(const Json &d) {
    auto year = number(d, "year", 0, 2100, true);
    if (year != "0" && std::stoi(year) < 1900) throw ApiError("Проверьте год выпуска.");
    return {field(d, "brand", 50, true), field(d, "model", 50, true), year,
            field(d, "fuel", 30), number(d, "engine", 0, 20), number(d, "mileage", 0, 10000000, true)};
}
void car_routes(httplib::Server &s) {
    route(s, "POST", "/api/account/import", Access::User, [](Context &c) {
        if (c.data.value("format",Json())!=1 || !c.data.value("cars",Json()).is_array() ||
            !c.data.value("records",Json()).is_array() || c.data["cars"].size()>500 || c.data["records"].size()>5000)
            throw ApiError("Неверный формат копии CarLog или слишком много записей.");
        c.db.sql("SELECT login FROM users WHERE login=$1 FOR UPDATE",{c.login});
        const auto fingerprint = hash(c.data.dump());
        if (!c.db.sql("SELECT 1 FROM account_imports WHERE login=$1 AND fingerprint=$2",{c.login,fingerprint}).empty())
            return Json{{"ok",true},{"repeated",true}};
        std::map<std::string,std::string> mapping;
        for (const auto &car : c.data["cars"]) {
            if (!car.is_object()) throw ApiError("Неверная запись автомобиля.");
            const auto old = number(car,"id",1,9e15,true,true);
            if (mapping.count(old)) throw ApiError("Повторяется номер автомобиля в файле.");
            auto args = values(car);
            args.push_back(c.login);
            auto created = c.db.sql("INSERT INTO cars(brand,model,year,fuel,engine,mileage,owner_login)"
                                   " VALUES($1,$2,$3,$4,$5,$6,$7) RETURNING id",args).front();
            mapping[old]=param(created["id"]);
        }
        for (auto rec : c.data["records"]) {
            if (!rec.is_object()) throw ApiError("Неверная запись журнала.");
            auto old = number(rec,"car",1,9e15,true,true);
            if (!mapping.count(old)) throw ApiError("Запись ссылается на отсутствующий автомобиль.");
            rec["car"]=mapping.at(old);
            auto created=create_record(c,rec);
            auto done=number(rec,"done",0,1,true);
            if (rec.value("kind",Json())=="reminder")
                c.db.sql("UPDATE records SET done=$1 WHERE id=$2",{done,param(created["id"])});
        }
        c.db.sql("INSERT INTO account_imports(login,fingerprint) VALUES($1,$2)",{c.login,fingerprint});
        return Json{{"ok",true},{"repeated",false}};
    });
    const std::string item = R"(/api/cars/([0-9]+))";
    route(s, "GET", "/api/cars", Access::User, [](Context &c) {
        return c.db.sql("SELECT id,brand,model,year,fuel,engine,mileage FROM cars WHERE owner_login=$1 ORDER BY id", {c.login});
    });
    route(s, "POST", "/api/cars", Access::User, [](Context &c) {
        auto args = values(c.data);
        args.push_back(c.login);
        args.push_back(field(c.data,"request_id",100));
        return c.db.sql("INSERT INTO cars(brand,model,year,fuel,engine,mileage,owner_login,request_key)"
                        " VALUES($1,$2,$3,$4,$5,$6,$7,NULLIF($8,''))"
                        " ON CONFLICT(owner_login,request_key) DO UPDATE SET request_key=EXCLUDED.request_key RETURNING id", args).front();
    }, 201);
    route(s, "PUT", item, Access::User, [](Context &c) {
        auto args = values(c.data);
        args.insert(args.end(), {c.login, c.id()});
        need(c.db.sql("UPDATE cars SET brand=$1,model=$2,year=$3,fuel=$4,engine=$5,mileage=$6"
                      " WHERE owner_login=$7 AND id=$8 RETURNING id", args), "Автомобиль не найден.");
        return ok();
    });
    route(s, "DELETE", item, Access::User, [](Context &c) {
        need(c.db.sql("DELETE FROM cars WHERE owner_login=$1 AND id=$2 RETURNING id", {c.login,c.id()}), "Автомобиль не найден.");
        return ok();
    });
}
