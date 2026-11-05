#include "core.hpp"

void stats_routes(httplib::Server &s) {
    route(s, "GET", "/api/stats/summary", Access::User, [](Context &c) {
        auto id = number({{"car",c.query("car")}}, "car", 1, 9e18, true, true);
        return need(c.db.sql(
            "SELECT c.id AS car,c.brand||' '||c.model AS name,c.mileage,"
            "COALESCE(sum(r.amount) FILTER(WHERE r.kind<>'reminder'),0) AS total_expenses,"
            "count(r.id) FILTER(WHERE r.kind<>'reminder') AS records_count,"
            "COALESCE(round((100*sum(r.liters) FILTER(WHERE r.kind='fuel') / NULLIF("
            "max(r.mileage) FILTER(WHERE r.kind='fuel')-min(r.mileage) FILTER(WHERE r.kind='fuel'),0))::numeric,2),0) AS consumption"
            " FROM cars c LEFT JOIN records r ON r.car=c.id WHERE c.id=$1 AND c.owner_login=$2 GROUP BY c.id",
            {id,c.login}), "Автомобиль не найден.");
    });
    route(s, "GET", "/api/stats/totals", Access::User, [](Context &c) {
        return c.db.sql(
            "SELECT COALESCE(sum(r.amount),0) AS total,COALESCE(sum(r.amount) FILTER(WHERE r.kind='fuel'),0) AS fuel,"
            "COALESCE(sum(r.amount) FILTER(WHERE r.kind='service'),0) AS service,"
            "COALESCE(sum(r.amount) FILTER(WHERE r.kind NOT IN ('fuel','service')),0) AS other"
            " FROM records r JOIN cars c ON c.id=r.car WHERE r.kind<>'reminder' AND c.owner_login=$1", {c.login}).front();
    });
}
