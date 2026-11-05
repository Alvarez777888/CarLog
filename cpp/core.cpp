#include "core.hpp"
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <cmath>
#include <iostream>

std::string env(const char *name, std::string fallback) {
    const auto *v = std::getenv(name);
    return v ? v : fallback;
}
static std::string hex(const unsigned char *bytes, size_t size) {
    const char *digits = "0123456789abcdef";
    std::string result;
    for (size_t i = 0; i < size; ++i) {
        result += digits[bytes[i] >> 4];
        result += digits[bytes[i] & 15];
    }
    return result;
}
static std::string sha(const std::string &s) {
    unsigned char bytes[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(s.data()), s.size(), bytes);
    return std::string(reinterpret_cast<char *>(bytes), sizeof bytes);
}
std::string hash(const std::string &s) {
    const auto bytes = sha(s);
    return hex(reinterpret_cast<const unsigned char *>(bytes.data()), bytes.size());
}
std::string password_hash(const std::string &salt, const std::string &pw, bool legacy) {
    if (legacy) {
        auto bytes = salt + pw;
        for (int i = 0; i < 5000; ++i) bytes = sha(bytes);
        return hex(reinterpret_cast<const unsigned char *>(bytes.data()), bytes.size());
    }
    unsigned char bytes[32];
    if (PKCS5_PBKDF2_HMAC(pw.data(), pw.size(), reinterpret_cast<const unsigned char *>(salt.data()),
                          salt.size(), 600000, EVP_sha256(), sizeof bytes, bytes) != 1)
        throw std::runtime_error("PBKDF2 failed");
    return "pbkdf2:" + hex(bytes, sizeof bytes);
}
std::string random_hex(int size) {
    std::vector<unsigned char> bytes(size);
    if (RAND_bytes(bytes.data(), size) != 1) throw std::runtime_error("RAND_bytes failed");
    return hex(bytes.data(), bytes.size());
}
std::string random_code() {
    uint32_t n;
    do {
        if (RAND_bytes(reinterpret_cast<unsigned char *>(&n), sizeof n) != 1)
            throw std::runtime_error("RAND_bytes failed");
    } while (n >= 4294000000u);
    return std::to_string(1000000 + n % 1000000).substr(1);
}
bool equal_secret(const std::string &a, const std::string &b) {
    return a.size() == b.size() && CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

Db::Db() : conn(PQconnectdb(env("CARLOG_DATABASE_URL").c_str()), PQfinish) {
    if (!conn || PQstatus(conn.get()) != CONNECTION_OK)
        throw ApiError("База данных недоступна.", 503, "database_offline");
    script("SET client_encoding='UTF8'; SET statement_timeout='5s'; SET lock_timeout='5s'");
}
static void check(PGresult *r) {
    if (!r) throw std::runtime_error("PostgreSQL returned no result");
    const auto status = PQresultStatus(r);
    if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) return;
    const auto *state = PQresultErrorField(r, PG_DIAG_SQLSTATE);
    if (state && std::string(state) == "23505")
        throw ApiError("Такая запись уже существует.", 409, "conflict");
    // SQLSTATE is safe to log. PostgreSQL's DETAIL can contain personal data.
    throw std::runtime_error(std::string("PostgreSQL SQLSTATE ") + (state ? state : "unknown"));
}
void Db::script(const std::string &query) {
    std::unique_ptr<PGresult, decltype(&PQclear)> r(PQexec(conn.get(), query.c_str()), PQclear);
    check(r.get());
}
Json Db::sql(const std::string &query, const Params &params) {
    std::vector<const char *> values;
    for (const auto &p : params) {
        if (p.find('\0') != std::string::npos) throw ApiError("Недопустимый символ.");
        values.push_back(p.c_str());
    }
    std::unique_ptr<PGresult, decltype(&PQclear)> result(
        PQexecParams(conn.get(), query.c_str(), values.size(), nullptr, values.data(), nullptr, nullptr, 0), PQclear);
    auto *r = result.get();
    check(r);
    auto rows = Json::array();
    for (int i = 0; i < PQntuples(r); ++i) {
        auto row = Json::object();
        for (int j = 0; j < PQnfields(r); ++j) {
            const auto *key = PQfname(r, j);
            if (PQgetisnull(r, i, j)) { row[key] = nullptr; continue; }
            const std::string s = PQgetvalue(r, i, j);
            switch (PQftype(r, j)) {
                case 16: row[key] = s == "t"; break;
                case 20: case 21: case 23: row[key] = std::stoll(s); break;
                case 700: case 701: case 1700: row[key] = std::stod(s); break;
                default: row[key] = s; break;
            }
        }
        rows.push_back(std::move(row));
    }
    return rows;
}
Json first(const Json &rows) { return rows.empty() ? Json(nullptr) : rows.front(); }
Json need(const Json &rows, const char *message, const char *code) {
    if (rows.empty()) throw ApiError(message, 404, code);
    return rows.front();
}
std::string param(const Json &v) { return v.is_string() ? v.get<std::string>() : v.dump(); }
std::string field(const Json &data, const char *key, size_t max, bool required) {
    const auto v = data.value(key, Json(nullptr));
    if (!v.is_null() && !v.is_string() && !v.is_number()) throw ApiError(std::string("Недопустимое поле: ") + key);
    auto s = v.is_null() ? std::string{} : param(v);
    const auto start = s.find_first_not_of(" \t\r\n");
    s = start == std::string::npos ? "" : s.substr(start, s.find_last_not_of(" \t\r\n") - start + 1);
    const auto chars = std::count_if(s.begin(), s.end(), [](unsigned char c) { return (c & 0xc0) != 0x80; });
    if ((required && s.empty()) || size_t(chars) > max || s.find('\0') != std::string::npos)
        throw ApiError(std::string("Проверьте поле «") + key + "».");
    return s;
}
std::string number(const Json &data, const char *key, double min, double max, bool integral, bool required) {
    auto s = field(data, key, 40, required);
    if (s.empty()) return "0";
    std::replace(s.begin(), s.end(), ',', '.');
    try {
        size_t end;
        if (integral) {
            auto n = std::stoll(s, &end);
            if (end == s.size() && n >= min && n <= max) return std::to_string(n);
        } else {
            auto n = std::stod(s, &end);
            if (end == s.size() && std::isfinite(n) && n >= min && n <= max) return Json(n).dump();
        }
    } catch (const std::logic_error &) {}
    throw ApiError(std::string("Проверьте число в поле «") + key + "».");
}
std::string day(const Json &data, const char *key, bool required) {
    const auto s = field(data, key, 10, required);
    if (s.empty()) return s;
    if (std::regex_match(s, std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}"))) {
        const auto y = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8));
        const int days[] = {31, 28 + (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)), 31,30,31,30,31,31,30,31,30,31};
        if (y && m >= 1 && m <= 12 && d >= 1 && d <= days[m-1]) return s;
    }
    throw ApiError(std::string("Некорректная дата в поле «") + key + "».");
}
std::string Context::query(const char *key) const { return request.get_param_value(key); }
std::string Context::id() const {
    return number({{"id", request.matches[1].str()}}, "id", 1, 9e18, true, true);
}
void route(httplib::Server &server, const std::string &method, const std::string &path,
           Access access, Handler handle, int status) {
    auto wrapped = [=](const httplib::Request &req, httplib::Response &res) {
        auto respond = [&](int code, Json body) {
            res.status = code;
            res.set_header("Cache-Control", "no-store");
            res.set_content(body.dump(), "application/json; charset=utf-8");
        };
        try {
            if (access == Access::Bot) {
                auto secret = env("CARLOG_BOT_SECRET");
                if (secret.empty() || !equal_secret(secret, req.get_header_value("X-Bot-Secret")))
                    throw ApiError("Доступ для бота запрещён.", 403, "forbidden");
            }
            Json data = Json::object();
            if (!req.body.empty()) data = Json::parse(req.body, nullptr, false);
            if (!data.is_object()) throw ApiError("Ожидается JSON-объект.");
            Db db;
            db.script("BEGIN");
            Context ctx{req, db, std::move(data), {}, {}};
            if (access == Access::User) {
                auto header = req.get_header_value("Authorization");
                if (header.rfind("Bearer ", 0) != 0) throw ApiError("Войдите в систему.", 401, "unauthorized");
                ctx.token = hash(header.substr(7));
                const auto row = first(db.sql(
                    "SELECT login FROM tokens WHERE token_hash=$1 AND expires_at>=to_char(now() AT TIME ZONE 'UTC','YYYY-MM-DD HH24:MI:SS')",
                    {ctx.token}));
                if (row.is_null()) throw ApiError("Войдите заново.", 401, "unauthorized");
                ctx.login = row.at("login").get<std::string>();
            }
            auto body = handle(ctx);
            db.script("COMMIT");
            respond(status, body);
        } catch (const ApiError &e) {
            respond(e.status, {{"error", e.what()}, {"code", e.code}});
        } catch (const std::exception &e) {
            std::cerr << method << ' ' << path << ": " << e.what() << '\n';
            respond(500, {{"error", "Ошибка обработки запроса."}, {"code", "internal"}});
        }
    };
    if (method == "GET") server.Get(path, wrapped);
    else if (method == "POST") server.Post(path, wrapped);
    else if (method == "PUT") server.Put(path, wrapped);
    else if (method == "DELETE") server.Delete(path, wrapped);
    else throw std::logic_error("Invalid HTTP method");
}
