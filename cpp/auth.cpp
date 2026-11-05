#include "core.hpp"

std::string clean_email(const Json &d) {
    auto email = field(d, "email", 120);
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });
    if (!email.empty() && !std::regex_match(email, std::regex(R"([^@\s]+@[^@\s]+\.[^@\s]+)")))
        throw ApiError("Введите корректный email.");
    return email;
}
static std::string password(const Json &d, const char *key, bool check_length) {
    auto value = d.value(key, Json(""));
    if (!value.is_string()) throw ApiError("Проверьте пароль.");
    auto pw = value.get<std::string>();
    if (pw.size() > 512 || (check_length && pw.size() < 6)) throw ApiError("Пароль должен содержать от 6 до 512 байт.");
    return pw;
}
void set_password(Db &db, const std::string &login, const std::string &pw) {
    auto salt = random_hex(16);
    db.sql("UPDATE users SET salt=$1,password=$2 WHERE login=$3", {salt, password_hash(salt, pw), login});
}
void auth_routes(httplib::Server &s) {
    route(s, "GET", "/api/account", Access::User, [](Context &c) {
        return c.db.sql("SELECT login,COALESCE(email,'') AS email,"
            "EXISTS(SELECT 1 FROM telegram_links WHERE login=users.login) AS telegram_linked"
            " FROM users WHERE login=$1",{c.login}).front();
    });
    route(s, "POST", "/api/account/password", Access::User, [](Context &c) {
        auto old=password(c.data,"old_password",false), next=password(c.data,"new_password",true);
        auto row=c.db.sql("SELECT salt,password FROM users WHERE login=$1 FOR UPDATE",{c.login}).front();
        const auto stored=param(row["password"]);
        if (!equal_secret(stored,password_hash(param(row["salt"]),old,stored.rfind("pbkdf2:",0)!=0)))
            throw ApiError("Текущий пароль неверен.",400,"bad_password");
        set_password(c.db,c.login,next);
        c.db.sql("DELETE FROM tokens WHERE login=$1 AND token_hash<>$2",{c.login,c.token});
        return ok();
    });
    route(s, "POST", "/api/register", Access::Public, [](Context &c) {
        auto login = field(c.data, "login", 50, true);
        if (std::count_if(login.begin(), login.end(), [](unsigned char ch){ return (ch & 0xc0) != 0x80; }) < 3)
            throw ApiError("Логин должен содержать минимум 3 символа.");
        auto pw = password(c.data, "password", true), salt = random_hex(16);
        c.db.sql("INSERT INTO users(login,salt,password,email) VALUES($1,$2,$3,$4)",
                 {login, salt, password_hash(salt, pw), clean_email(c.data)});
        return ok();
    }, 201);
    route(s, "POST", "/api/login", Access::Public, [](Context &c) {
        auto login = field(c.data, "login", 50, true), pw = password(c.data, "password", false);
        c.db.sql("SELECT pg_advisory_xact_lock(hashtextextended($1,1))", {login});
        c.db.sql("DELETE FROM login_attempts WHERE attempted_at<now()-interval '5 minutes'");
        auto count = c.db.sql("SELECT count(*) FROM login_attempts WHERE login=$1", {login})[0]["count"];
        if (count.get<int>() >= 5) throw ApiError("Слишком много попыток. Попробуйте позже.", 429, "locked");
        auto row = first(c.db.sql("SELECT salt,password FROM users WHERE login=$1 FOR UPDATE", {login}));
        bool legacy = !row.is_null() && param(row["password"]).rfind("pbkdf2:", 0) != 0;
        auto actual = password_hash(row.is_null() ? "missing-account" : param(row["salt"]), pw, legacy);
        if (row.is_null() || !equal_secret(param(row["password"]), actual)) {
            c.db.sql("INSERT INTO login_attempts(login) VALUES($1)", {login});
            c.db.script("COMMIT");
            throw ApiError("Неверный логин или пароль.", 401, "bad_credentials");
        }
        if (legacy) set_password(c.db, login, pw);
        c.db.sql("DELETE FROM login_attempts WHERE login=$1", {login});
        auto token = random_hex(32);
        c.db.sql("INSERT INTO tokens(token_hash,login,expires_at) VALUES($1,$2,"
                 "to_char((now() AT TIME ZONE 'UTC')+interval '30 days','YYYY-MM-DD HH24:MI:SS'))", {hash(token), login});
        return Json{{"token", token}, {"login", login}};
    });
    route(s, "POST", "/api/logout", Access::User, [](Context &c) {
        c.db.sql("DELETE FROM tokens WHERE token_hash=$1", {c.token});
        return ok();
    });
    route(s, "POST", "/api/account/email", Access::User, [](Context &c) {
        c.db.sql("UPDATE users SET email=$1 WHERE login=$2", {clean_email(c.data), c.login});
        return ok();
    });
}
