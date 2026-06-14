/**
 * @file database_connection.cpp
 * @version 1.2.0
 */

#include "include/database_connection.h"
#include "include/config_manager.h"
#include "include/logger.h"
#include <iostream>

DatabaseConnection::DatabaseConnection() : conn(nullptr) {}

DatabaseConnection::~DatabaseConnection() {
    close();
}

bool DatabaseConnection::connect(const char* host, const char* user, const char* password, 
                                 const char* database, unsigned int port) {
    MYSQL* tmp = mysql_init(nullptr);
    if (!tmp) {
        Logger::getInstance().log("mysql_init() failed", Logger::LogType::Error);
        return false;
    }

    conn = mysql_real_connect(tmp, host, user, password, database, port, nullptr, 0);
    if (!conn) {
        std::string err_msg = "Błąd połączenia z MariaDB: " + std::string(mysql_error(tmp));
        Logger::getInstance().log(err_msg, Logger::LogType::Error);
        mysql_close(tmp);
        return false;
    }

    return true;
}

MYSQL_RES* DatabaseConnection::query(const char* sql) {
    if (!conn) {
        Logger::getInstance().log("Próba wykonania zapytania bez aktywnego połączenia", Logger::LogType::Error);
        return nullptr;
    }
    if (mysql_query(conn, sql)) {
        std::string err_msg = "Błąd zapytania: " + std::string(mysql_error(conn)) + " | SQL: " + sql;
        Logger::getInstance().log(err_msg, Logger::LogType::Error);
        return nullptr;
    }
    return mysql_store_result(conn);
}

void DatabaseConnection::close() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

