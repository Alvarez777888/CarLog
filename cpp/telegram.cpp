#include "core.hpp"

static std::string chat(const Json &d) { return number(d,"chat_id",1,9e18,true,true); }
static std::string chat_query(Context &c) { return chat({{"chat_id",c.query("chat_id")}}); }
static std::string linked(Context &c, const std::string &id) {
    return param(need(c.db.sql("SELECT login FROM telegram_links WHERE chat_id=$1 FOR SHARE", {id}),
                      "Telegram не привязан к аккаунту.", "not_linked")["login"]);
}
void telegram_routes(httplib::Server &s) {
    const auto user = Access::User, bot = Access::Bot;
    auto lead = number({{"days",env("CARLOG_REMIND_DAYS","1")}}, "days", 0, 365, true, true);
    route(s,"POST","/api/telegram/link-code",user,[](Context &c) {
        c.db.sql("SELECT login FROM users WHERE login=$1 FOR UPDATE", {c.login});
        c.db.sql("DELETE FROM link_codes WHERE login=$1 OR expires_at<to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS')", {c.login});
        for (int i = 0; i < 8; ++i) {
            const auto code = random_code();
            auto rows = c.db.sql("INSERT INTO link_codes(code,login,expires_at) VALUES($1,$2,"
                                 "to_char((now() AT TIME ZONE 'UTC')+interval '10 minutes','YYYY-MM-DD HH24:MI:SS'))"
                                 " ON CONFLICT DO NOTHING RETURNING code", {code,c.login});
            if (!rows.empty()) {
                auto name=env("CARLOG_BOT_USERNAME");
                auto url=std::regex_match(name,std::regex("[A-Za-z0-9_]{5,32}")) ?
                    "https://t.me/"+name+"?start=link_"+code : "";
                return Json{{"code",code},{"valid_minutes",10},{"bot_url",url}};
            }
        }
        throw ApiError("Повторите запрос кода.",503);
    });
    route(s,"POST","/api/telegram/unlink",user,[](Context &c) {
        c.db.sql("DELETE FROM outbox WHERE chat_id IN (SELECT chat_id FROM telegram_links WHERE login=$1)", {c.login});
        c.db.sql("DELETE FROM telegram_links WHERE login=$1", {c.login});
        c.db.sql("DELETE FROM link_codes WHERE login=$1", {c.login});
        return ok();
    });
    route(s,"POST","/api/bot/link",bot,[](Context &c) {
        const auto id = chat(c.data), code = field(c.data,"code",6,true);
        c.db.sql("SELECT pg_advisory_xact_lock(hashtextextended($1,2))", {id});
        c.db.sql("DELETE FROM link_attempts WHERE attempted_at<now()-interval '10 minutes'");
        if (c.db.sql("SELECT count(*) FROM link_attempts WHERE chat_id=$1",{id})[0]["count"].get<int>() >= 5)
            throw ApiError("Слишком много попыток. Попробуйте позже.",429,"locked");
        const auto row = first(c.db.sql("DELETE FROM link_codes WHERE code=$1 AND "
            "expires_at>=to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS') RETURNING login", {code}));
        if (row.is_null()) {
            c.db.sql("INSERT INTO link_attempts(chat_id) VALUES($1)", {id});
            c.db.script("COMMIT");
            throw ApiError("Код неверный или устарел.",400,"bad_code");
        }
        auto login = param(row["login"]);
        c.db.sql("DELETE FROM link_attempts WHERE chat_id=$1", {id});
        c.db.sql("DELETE FROM outbox WHERE chat_id=$1 AND EXISTS(SELECT 1 FROM telegram_links WHERE chat_id=$1 AND login<>$2)",{id,login});
        c.db.sql("DELETE FROM telegram_links WHERE chat_id=$1 AND login<>$2", {id,login});
        c.db.sql("INSERT INTO telegram_links(chat_id,login) VALUES($1,$2) ON CONFLICT DO NOTHING", {id,login});
        return row;
    });
    route(s,"POST","/api/bot/unlink",bot,[](Context &c) {
        const auto id = chat(c.data);
        c.db.sql("DELETE FROM outbox WHERE chat_id=$1", {id});
        c.db.sql("DELETE FROM telegram_links WHERE chat_id=$1", {id});
        return ok();
    });
    route(s,"GET","/api/bot/cars",bot,[](Context &c) {
        return c.db.sql("SELECT id,brand,model,mileage FROM cars WHERE owner_login=$1 ORDER BY id", {linked(c,chat_query(c))});
    });
    route(s,"POST","/api/bot/fuel",bot,[](Context &c) {
        const auto id = chat(c.data);
        c.login = linked(c,id);
        if (!c.data.contains("car") || c.data["car"].is_null() || c.data["car"] == 0) {
            auto rows = c.db.sql("SELECT id FROM cars WHERE owner_login=$1 LIMIT 2", {c.login});
            if (rows.size() != 1) throw ApiError("Укажите автомобиль.",400,"car_required");
            c.data["car"] = rows.front()["id"];
        }
        c.data["kind"] = "fuel";
        if (!c.data.contains("day")) c.data["day"] = c.db.sql("SELECT current_date::text AS day").front()["day"];
        if (field(c.data,"comment",300).empty()) c.data["comment"] = "Добавлено через Telegram";
        auto key = field(c.data,"source_key",100);
        return create_record(c,c.data,key.empty() ? "" : "telegram:" + id + ":" + key);
    },201);
    route(s,"GET","/api/bot/reminders/due",bot,[lead](Context &c) {
        return c.db.sql(
            "WITH due AS (SELECT r.id,r.day,r.description,r.reminder_version AS version,c.brand,c.model,t.chat_id,"
            " CASE WHEN r.day<=current_date::text THEN 2 ELSE 1 END AS stage"
            " FROM records r JOIN cars c ON c.id=r.car JOIN telegram_links t ON t.login=c.owner_login"
            " WHERE r.kind='reminder' AND r.done=0 AND r.notified=0 AND r.day<=(current_date+$1::int)::text)"
            " SELECT due.* FROM due LEFT JOIN reminder_deliveries d ON"
            " (d.record_id,d.chat_id,d.version,d.stage)=(due.id,due.chat_id,due.version,due.stage)"
            " WHERE d.record_id IS NULL ORDER BY due.day,due.id,due.chat_id LIMIT 100", {lead});
    });
    route(s,"POST",R"(/api/bot/reminders/([0-9]+)/notified)",bot,[](Context &c) {
        auto version = number(c.data,"version",1,9e18,true,true);
        auto stage = number(c.data,"stage",1,2,true,true);
        c.db.sql("INSERT INTO reminder_deliveries(record_id,chat_id,version,stage)"
                 " SELECT r.id,t.chat_id,r.reminder_version,$4 FROM records r JOIN cars c ON c.id=r.car"
                 " JOIN telegram_links t ON t.login=c.owner_login WHERE r.id=$1 AND t.chat_id=$2"
                 " AND r.reminder_version=$3 AND r.kind='reminder' ON CONFLICT DO NOTHING",
                 {c.id(),chat(c.data),version,stage});
        return ok();
    });
    route(s,"POST",R"(/api/bot/reminders/([0-9]+)/done)",bot,[](Context &c) {
        auto login=linked(c,chat(c.data));
        need(c.db.sql("UPDATE records r SET done=1 FROM cars c WHERE r.car=c.id AND r.id=$1"
                      " AND c.owner_login=$2 AND r.kind='reminder' RETURNING r.id",{c.id(),login}),
             "Напоминание не найдено.");
        return ok();
    });
    route(s,"GET","/api/bot/reminders/upcoming",bot,[](Context &c) {
        return c.db.sql("SELECT r.id,r.day,r.description,c.brand,c.model FROM records r JOIN cars c ON c.id=r.car"
                        " WHERE r.kind='reminder' AND r.done=0 AND c.owner_login=$1 ORDER BY r.day LIMIT 10",
                        {linked(c,chat_query(c))});
    });
    route(s,"GET","/api/bot/outbox",bot,[](Context &c) {
        return c.db.sql("SELECT o.id,o.chat_id,o.text FROM outbox o JOIN telegram_links t ON t.chat_id=o.chat_id"
                        " WHERE o.sent=0 AND o.created_at>=to_char((now() AT TIME ZONE 'UTC')-interval '15 minutes','YYYY-MM-DD HH24:MI:SS')"
                        " ORDER BY o.id LIMIT 20");
    });
    route(s,"POST",R"(/api/bot/outbox/([0-9]+)/sent)",bot,[](Context &c) {
        c.db.sql("DELETE FROM outbox WHERE id=$1 OR created_at<to_char((now() AT TIME ZONE 'UTC')-interval '1 day','YYYY-MM-DD HH24:MI:SS')",{c.id()});
        return ok();
    });
}
