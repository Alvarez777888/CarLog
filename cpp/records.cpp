#include "core.hpp"
#include <cmath>

static const std::string columns = "r.id,r.car,r.kind,r.day,r.category,r.description,r.amount,r.mileage,"
                                   "r.liters,r.price,r.place,r.comment,r.next_day,r.done";
static Params values(const Json &d, const std::string &kind) {
    if (kind != "fuel" && kind != "expense" && kind != "service" && kind != "reminder")
        throw ApiError("Неизвестный тип записи.");
    auto category = field(d, "category", 50);
    auto desc = field(d, "description", 200, kind == "reminder");
    auto amount = number(d, "amount", 0, 100000000);
    auto liters = number(d, "liters", 0, 1000);
    auto price = number(d, "price", 0, 100000);
    if (kind == "fuel") {
        const double l = std::stod(liters), p = std::stod(price), a = std::stod(amount);
        if (l <= 0 || (p <= 0 && a <= 0)) throw ApiError("Укажите литры и цену или сумму заправки.");
        if (a <= 0) amount = Json(std::round(l * p * 100) / 100).dump();
        if (p <= 0) price = Json(std::round(a / l * 100) / 100).dump();
        if (std::stod(amount) <= 0 || std::stod(price) <= 0 || std::stod(price) > 100000)
            throw ApiError("Проверьте сумму и цену заправки.");
        if (category.empty()) category = "Топливо";
    }
    return {day(d, "day"), category, desc, amount, number(d, "mileage", 0, 10000000, true),
            liters, price, field(d, "place", 100), field(d, "comment", 300), day(d, "next_day", false)};
}
static void mileage(Context &c, const std::string &car, const std::string &km) {
    c.db.sql("UPDATE cars SET mileage=GREATEST(mileage,$1::integer) WHERE id=$2", {km,car});
}
Json create_record(Context &c, const Json &data, const std::string &source) {
    auto car = number(data, "car", 1, 9e18, true, true);
    need(c.db.sql("SELECT id FROM cars WHERE id=$1 AND owner_login=$2 FOR UPDATE", {car,c.login}), "Автомобиль не найден.");
    auto kind = field(data, "kind", 20, true);
    auto args = values(data, kind);
    args.insert(args.end(), {car, kind, source});
    auto rows = c.db.sql("INSERT INTO records(day,category,description,amount,mileage,liters,price,place,comment,next_day,car,kind,source_key)"
                         " VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,NULLIF($13,''))"
                         " ON CONFLICT(car,source_key) DO NOTHING RETURNING id,liters,price,amount,place,day", args);
    if (rows.empty())
        return c.db.sql("SELECT id,liters,price,amount,place,day FROM records WHERE car=$1 AND source_key=$2", {car,source}).front();
    mileage(c, car, args[4]);
    return rows.front();
}
static Json lock_record(Context &c) {
    return need(c.db.sql("SELECT r.car,r.kind FROM records r JOIN cars c ON c.id=r.car "
                         "WHERE r.id=$1 AND c.owner_login=$2 FOR UPDATE OF r,c", {c.id(),c.login}), "Запись не найдена.");
}
void record_routes(httplib::Server &s) {
    const std::string item = R"(/api/records/([0-9]+))";
    route(s, "GET", "/api/account/export", Access::User, [](Context &c) {
        auto row = c.db.sql(
            "SELECT json_build_object('format',1,'login',$1::text,"
            "'cars',(SELECT COALESCE(json_agg(x),'[]'::json) FROM "
            "(SELECT id,brand,model,year,fuel,engine,mileage FROM cars WHERE owner_login=$1 ORDER BY id) x),"
            "'records',(SELECT COALESCE(json_agg(x),'[]'::json) FROM "
            "(SELECT r.id,r.car,r.kind,r.day,r.category,r.description,r.amount,r.mileage,r.liters,r.price,"
            "r.place,r.comment,r.next_day,r.done FROM records r JOIN cars c ON c.id=r.car"
            " WHERE c.owner_login=$1 ORDER BY r.id) x))::text AS payload", {c.login}).front();
        return Json::parse(param(row["payload"]));
    });

    route(s, "GET", "/api/records", Access::User, [](Context &c) {
        const auto car = c.query("car"), kind = c.query("kind");
        const auto id = car.empty() ? "" : number({{"car", car}}, "car", 1, 9e18, true, true);
        return c.db.sql("SELECT " + columns + " FROM records r JOIN cars c ON c.id=r.car WHERE c.owner_login=$1 "
                        "AND ($2='' OR r.kind=$2) AND (NULLIF($3,'') IS NULL OR r.car=NULLIF($3,'')::bigint)"
                        " ORDER BY r.day DESC,r.id DESC", {c.login,kind,id});
    });
    route(s, "POST", "/api/records", Access::User, [](Context &c) {
        auto request = field(c.data,"request_id",100);
        return Json{{"id", create_record(c,c.data,request.empty() ? "" : "client:"+c.login+":"+request).at("id")}};
    }, 201);
    route(s, "PUT", item, Access::User, [](Context &c) {
        const auto old = lock_record(c);
        auto args = values(c.data, param(old["kind"]));
        args.push_back(c.id());
        c.db.sql("UPDATE records SET day=$1,category=$2,description=$3,amount=$4,mileage=$5,liters=$6,"
                 "price=$7,place=$8,comment=$9,next_day=$10,notified=0,reminder_version=reminder_version+1 WHERE id=$11", args);
        mileage(c,param(old["car"]),args[4]);
        return ok();
    });
    route(s, "DELETE", item, Access::User, [](Context &c) {
        need(c.db.sql("DELETE FROM records r USING cars c WHERE r.id=$1 AND r.car=c.id AND c.owner_login=$2 RETURNING r.id",
                      {c.id(),c.login}), "Запись не найдена.");
        return ok();
    });
    route(s, "POST", item + "/toggle", Access::User, [](Context &c) {
        if (lock_record(c)["kind"] != "reminder") throw ApiError("Это не напоминание.");
        c.db.sql("UPDATE records SET done=1-done WHERE id=$1", {c.id()});
        return ok();
    });
}
