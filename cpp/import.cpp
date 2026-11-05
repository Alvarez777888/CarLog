#include "core.hpp"
#include <sqlite3.h>
#include <iostream>

using Sqlite = std::unique_ptr<sqlite3,decltype(&sqlite3_close)>;
using Statement = std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
static Statement prepare(sqlite3 *db,const std::string &query) {
    sqlite3_stmt *raw = nullptr;
    auto rc = sqlite3_prepare_v2(db,query.c_str(),-1,&raw,nullptr);
    Statement stmt(raw,sqlite3_finalize);
    if (rc != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db));
    return stmt;
}
static void copy(sqlite3 *source,Db &target,const std::string &table,const Params &allowed,bool optional=false) {
    auto exists = prepare(source,"SELECT 1 FROM sqlite_master WHERE type='table' AND name='"+table+"'");
    auto rc = sqlite3_step(exists.get());
    if (optional && rc == SQLITE_DONE) return;
    auto stmt = prepare(source,"SELECT * FROM "+table);
    Params columns;
    std::vector<int> indexes;
    for (const auto &column : allowed) {
        for (int i=0;i<sqlite3_column_count(stmt.get());++i) {
            if (column == sqlite3_column_name(stmt.get(),i)) {
                columns.push_back(column);
                indexes.push_back(i);
                break;
            }
        }
    }
    if (columns.empty()) throw std::runtime_error("Incompatible SQLite table: "+table);
    if (table == "categories") target.sql("DELETE FROM categories");
    std::string prefix = "INSERT INTO "+table+"(";
    for (size_t i=0;i<columns.size();++i) prefix += (i ? "," : "") + columns[i];
    prefix += ") VALUES(";
    int count=0;
    while ((rc=sqlite3_step(stmt.get())) == SQLITE_ROW) {
        Params args;
        auto sql = prefix;
        for (size_t i=0;i<columns.size();++i) {
            if (i) sql += ',';
            const auto col = indexes[i];
            if (sqlite3_column_type(stmt.get(),col) == SQLITE_NULL) sql += "NULL";
            else {
                const auto *value = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(),col));
                if (!value) throw std::runtime_error("Cannot read SQLite value");
                args.emplace_back(value,sqlite3_column_bytes(stmt.get(),col));
                sql += '$'+std::to_string(args.size());
            }
        }
        target.sql(sql+")",args);
        ++count;
    }
    if (rc != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(source));
    std::cout << table << ": " << count << '\n';
}
int main(int argc,char **argv) try {
    if (argc != 2 || env("CARLOG_DATABASE_URL").empty())
        throw std::runtime_error("Set CARLOG_DATABASE_URL and run carlog-import PATH_TO_SQLITE");
    sqlite3 *raw = nullptr;
    auto rc = sqlite3_open_v2(argv[1],&raw,SQLITE_OPEN_READONLY,nullptr);
    Sqlite source(raw,sqlite3_close);
    if (rc != SQLITE_OK) throw std::runtime_error("Cannot open SQLite read-only");
    if (sqlite3_exec(source.get(),"BEGIN",nullptr,nullptr,nullptr) != SQLITE_OK)
        throw std::runtime_error("Cannot open a SQLite snapshot");
    Db db;
    db.script("BEGIN; LOCK TABLE users,cars,records,categories,telegram_links IN ACCESS EXCLUSIVE MODE");
    if (!db.sql("SELECT 1 FROM users LIMIT 1").empty()) throw std::runtime_error("Use an empty PostgreSQL database");
    copy(source.get(),db,"users",{"login","salt","password","email"});
    copy(source.get(),db,"cars",{"id","brand","model","year","fuel","engine","mileage","owner_login"});
    copy(source.get(),db,"records",{"id","car","kind","day","category","description","amount","mileage",
                         "liters","price","place","comment","next_day","done","notified"});
    copy(source.get(),db,"categories",{"id","name"},true);
    copy(source.get(),db,"telegram_links",{"chat_id","login"},true);
    for (const std::string name : {"cars","records","categories"}) {
        auto next = db.sql("SELECT GREATEST(COALESCE(max(id),0)+1,1) AS next FROM "+name).front()["next"];
        db.sql("ALTER TABLE "+name+" ALTER COLUMN id RESTART WITH "+param(next));
    }
    db.script("COMMIT");
    std::cout << "Import committed\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
