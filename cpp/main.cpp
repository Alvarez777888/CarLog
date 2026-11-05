#include "core.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) try {
    if (env("CARLOG_DATABASE_URL").empty()) throw std::runtime_error("Set CARLOG_DATABASE_URL");
    if (argc == 2 && std::string(argv[1]) == "--init") {
        std::ifstream input(env("CARLOG_SCHEMA","cpp/schema.sql"));
        if (!input) throw std::runtime_error("Cannot open cpp/schema.sql");
        Db().script(std::string(std::istreambuf_iterator<char>(input),{}));
        std::cout << "Schema ready\n";
        return 0;
    }
    if (argc != 1) throw std::runtime_error("Usage: carlog-server [--init]");
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) throw std::runtime_error("Cannot initialize libcurl");
    httplib::Server server;
    server.new_task_queue = [] { return new httplib::ThreadPool(8,64); };
    server.set_payload_max_length(1024*1024);
    server.set_read_timeout(10);
    server.set_write_timeout(10);
    auto failed = Json::array();
    const std::pair<const char *,void (*)(httplib::Server &)> modules[] = {
        {"auth",auth_routes},{"cars",car_routes},{"records",record_routes},
        {"stats",stats_routes},{"telegram",telegram_routes},{"recovery",recovery_routes}
    };
    for (auto &[name,configure] : modules) {
        try { configure(server); }
        catch (const std::exception &e) { failed.push_back(name); std::cerr << name << ": " << e.what() << '\n'; }
    }
    server.Get("/api/health",[failed](const auto &,auto &res) {
        res.set_content(Json{{"status",failed.empty() ? "ok" : "degraded"},{"failed_modules",failed}}.dump(),"application/json");
    });
    server.set_error_handler([](const auto &,auto &res) {
        if (!res.body.empty()) return;
        res.set_content(Json{{"error","Запрос не выполнен."},{"code",res.status==404 ? "not_found" : "http_error"}}.dump(),"application/json");
    });
    auto host = env("CARLOG_HOST","127.0.0.1");
    auto port = std::stoi(env("CARLOG_PORT","8000"));
    std::cout << "CarLog listening on " << host << ':' << port << std::endl;
    if (!server.listen(host,port)) throw std::runtime_error("Cannot listen on configured address");
    curl_global_cleanup();
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
