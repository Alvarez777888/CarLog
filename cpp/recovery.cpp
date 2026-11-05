#include "core.hpp"
#include <curl/curl.h>
#include <cstring>
#include <iostream>

static std::string method(Context &c) {
    auto m = field(c.data,"method",15,true);
    if (m != "email" && m != "telegram") throw ApiError("Неизвестный способ восстановления.");
    return m;
}
static Json user(Context &c) {
    return first(c.db.sql("SELECT login,COALESCE(email,'') AS email FROM users WHERE lower(login)=lower($1)"
                          " OR (email<>'' AND lower(email)=lower($1))"
                          " ORDER BY (lower(login)=lower($1)) DESC,login LIMIT 1 FOR UPDATE",
                          {field(c.data,"login_or_email",120,true)}));
}
static size_t smtp_read(char *out,size_t size,size_t count,void *data) {
    auto &body = *static_cast<std::string_view *>(data);
    auto n = std::min(size*count,body.size());
    std::memcpy(out,body.data(),n);
    body.remove_prefix(n);
    return n;
}
static bool send_email(const std::string &to,const std::string &code) {
    auto login = env("CARLOG_SMTP_USER"), pw = env("CARLOG_SMTP_PASSWORD"), from = env("CARLOG_SMTP_FROM");
    if (from.empty()) from = login;
    if (login.empty() || pw.empty() || from.find_first_of("\r\n") != std::string::npos ||
        to.find_first_of("\r\n") != std::string::npos) return false;
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
    if (!curl) return false;
    std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> recipients(
        curl_slist_append(nullptr,("<"+to+">").c_str()),curl_slist_free_all);
    if (!recipients) return false;
    const auto port = env("CARLOG_SMTP_PORT","587");
    const auto scheme = port == "465" ? "smtps://" : "smtp://";
    const auto url = scheme + env("CARLOG_SMTP_HOST","smtp.gmail.com") + ":" + port;
    auto message = "From: <"+from+">\r\nTo: <"+to+">\r\nSubject: Car Log recovery code\r\n\r\n"
                   "Your Car Log recovery code is "+code+". It expires in 15 minutes.\r\n";
    std::string_view payload(message);
    curl_easy_setopt(curl.get(),CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl.get(),CURLOPT_PROTOCOLS_STR,"smtp,smtps");
    curl_easy_setopt(curl.get(),CURLOPT_USE_SSL,CURLUSESSL_ALL);
    curl_easy_setopt(curl.get(),CURLOPT_USERNAME,login.c_str());
    curl_easy_setopt(curl.get(),CURLOPT_PASSWORD,pw.c_str());
    curl_easy_setopt(curl.get(),CURLOPT_MAIL_FROM,from.c_str());
    curl_easy_setopt(curl.get(),CURLOPT_MAIL_RCPT,recipients.get());
    curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT,10L);
    curl_easy_setopt(curl.get(),CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(curl.get(),CURLOPT_READFUNCTION,smtp_read);
    curl_easy_setopt(curl.get(),CURLOPT_READDATA,&payload);
    curl_easy_setopt(curl.get(),CURLOPT_UPLOAD,1L);
    const auto result = curl_easy_perform(curl.get());
    if (result != CURLE_OK)
        std::cerr << "SMTP delivery failed: curl code " << static_cast<int>(result)
                  << " (" << curl_easy_strerror(result) << ")\n";
    return result == CURLE_OK;
}
void recovery_routes(httplib::Server &s) {
    auto secret = env("CARLOG_RECOVERY_SECRET");
    if (secret.size() < 32) throw std::runtime_error("Set CARLOG_RECOVERY_SECRET (at least 32 characters)");
    route(s,"POST","/api/recovery/request",Access::Public,[secret](Context &c) {
        auto m = method(c);
        auto account = user(c);
        if (account.is_null()) return ok();
        auto login = param(account["login"]), email = param(account["email"]);
        auto count = c.db.sql("SELECT count(*) AS hourly,count(*) FILTER (WHERE created_at>=to_char("
            "(now() AT TIME ZONE 'UTC')-interval '1 minute','YYYY-MM-DD HH24:MI:SS')) AS recent"
            " FROM recovery_codes WHERE login=$1 AND method IN ('email','telegram') AND created_at>=to_char("
            "(now() AT TIME ZONE 'UTC')-interval '1 hour','YYYY-MM-DD HH24:MI:SS')",{login}).front();
        if (count["hourly"].get<int>() >= 5 || count["recent"].get<int>() > 0) return ok();
        auto link = first(c.db.sql("SELECT chat_id FROM telegram_links WHERE login=$1 ORDER BY chat_id LIMIT 1 FOR SHARE",{login}));
        if ((m == "email" && email.empty()) || (m == "telegram" && link.is_null())) return ok();
        auto code = random_code();
        c.db.sql("UPDATE recovery_codes SET used=1 WHERE login=$1 AND used=0",{login});
        auto rows = c.db.sql("INSERT INTO recovery_codes(login,method,token_hash,created_at,expires_at)"
            " VALUES($1,$2,$3,to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS'),"
            " to_char((now() AT TIME ZONE 'UTC')+interval '15 minutes','YYYY-MM-DD HH24:MI:SS')) RETURNING id",
            {login,m,hash(secret+":"+code)});
        if (m == "telegram") {
            c.db.sql("INSERT INTO outbox(chat_id,text,created_at) VALUES($1,$2,to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS'))",
                     {param(link["chat_id"]),"Код восстановления Car Log: "+code+". Действует 15 минут."});
        } else if (!send_email(email,code)) {
            c.db.sql("UPDATE recovery_codes SET used=1 WHERE id=$1",{param(rows.front()["id"])});
        }
        return ok();
    });
    route(s,"POST","/api/recovery/verify",Access::Public,[secret](Context &c) {
        auto m = method(c);
        auto code = field(c.data,"code",6,true);
        auto account = user(c);
        Json row = nullptr;
        if (!account.is_null()) {
            row = first(c.db.sql("SELECT id,token_hash,used,attempts,expires_at>=to_char(now() AT TIME ZONE 'UTC',"
                "'YYYY-MM-DD HH24:MI:SS') AS valid FROM recovery_codes WHERE login=$1 AND method=$2 ORDER BY id DESC LIMIT 1",
                {param(account["login"]),m}));
        }
        if (row.is_null() || row["used"] != 0 || !row["valid"].get<bool>()) throw ApiError("Код недействителен или истёк.",400,"bad_code");
        if (row["attempts"].get<int>() >= 5) throw ApiError("Запросите новый код.",429,"locked");
        if (!equal_secret(param(row["token_hash"]),hash(secret+":"+code))) {
            c.db.sql("UPDATE recovery_codes SET attempts=attempts+1 WHERE id=$1",{param(row["id"])});
            c.db.script("COMMIT");
            throw ApiError("Неверный код.",400,"bad_code");
        }
        auto token = random_hex(32);
        c.db.sql("UPDATE recovery_codes SET used=1 WHERE id=$1",{param(row["id"])});
        c.db.sql("INSERT INTO recovery_codes(login,method,token_hash,created_at,expires_at)"
                 " VALUES($1,'reset',$2,to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS'),"
                 " to_char((now() AT TIME ZONE 'UTC')+interval '10 minutes','YYYY-MM-DD HH24:MI:SS'))",
                 {param(account["login"]),hash(token)});
        return Json{{"ok",true},{"reset_token",token}};
    });
    route(s,"POST","/api/recovery/reset",Access::Public,[](Context &c) {
        auto token = hash(field(c.data,"reset_token",128,true));
        auto pw = c.data.value("new_password",Json(""));
        if (!pw.is_string() || pw.get_ref<const std::string &>().size() < 6 ||
            pw.get_ref<const std::string &>().size() > 512) throw ApiError("Пароль должен содержать от 6 до 512 байт.");
        auto row = first(c.db.sql("SELECT u.login FROM recovery_codes r JOIN users u ON u.login=r.login"
                                  " WHERE r.method='reset' AND r.token_hash=$1 FOR UPDATE OF u",{token}));
        if (row.is_null()) throw ApiError("Сессия восстановления недействительна.",400,"bad_token");
        auto login = param(row["login"]);
        auto changed = c.db.sql("UPDATE recovery_codes SET used=1 WHERE login=$1 AND method='reset' AND token_hash=$2"
            " AND used=0 AND expires_at>=to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS') RETURNING id",{login,token});
        if (changed.empty()) throw ApiError("Сессия восстановления недействительна.",400,"bad_token");
        set_password(c.db,login,pw.get<std::string>());
        c.db.sql("DELETE FROM tokens WHERE login=$1",{login});
        return ok();
    });
}
