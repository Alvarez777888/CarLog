#pragma once
#include "httplib.h"
#include "json.hpp"
#include <libpq-fe.h>
#include <functional>
#include <memory>
#include <stdexcept>

using Json = nlohmann::json;
using Params = std::vector<std::string>;
struct ApiError : std::runtime_error {
    int status;
    std::string code;
    explicit ApiError(std::string message, int status = 400, std::string code = "invalid")
        : runtime_error(message), status(status), code(std::move(code)) {}
};
std::string env(const char *name, std::string fallback = {});
std::string hash(const std::string &value);
std::string random_hex(int bytes);
std::string random_code();
std::string password_hash(const std::string &salt, const std::string &password, bool legacy = false);
bool equal_secret(const std::string &a, const std::string &b);
std::string field(const Json &data, const char *key, size_t max = 200, bool required = false);
std::string number(const Json &data, const char *key, double min, double max, bool integral = false, bool required = false);
std::string day(const Json &data, const char *key, bool required = true);

struct Db {
    std::unique_ptr<PGconn, decltype(&PQfinish)> conn;
    Db();
    Json sql(const std::string &query, const Params &params = {});
    void script(const std::string &query);
};
Json first(const Json &rows);
Json need(const Json &rows, const char *message, const char *code = "not_found");
std::string param(const Json &value);
struct Context {
    const httplib::Request &request;
    Db &db;
    Json data;
    std::string login, token;
    std::string query(const char *key) const;
    std::string id() const;
};
enum class Access { Public, User, Bot };
using Handler = std::function<Json(Context &)>;
void route(httplib::Server &, const std::string &method, const std::string &path,
           Access, Handler, int status = 200);
void auth_routes(httplib::Server &);
void car_routes(httplib::Server &);
void record_routes(httplib::Server &);
void stats_routes(httplib::Server &);
void telegram_routes(httplib::Server &);
void recovery_routes(httplib::Server &);
std::string clean_email(const Json &);
void set_password(Db &, const std::string &, const std::string &);
Json create_record(Context &, const Json &, const std::string &source = {});
inline Json ok() { return {{"ok", true}}; }
