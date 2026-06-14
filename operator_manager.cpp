/**
 * @file operator_manager.cpp
 * @brief Automatyczne zarządzanie statusem nieaktywnych operatorów w osobnym wątku.
 * @version 1.2.0
 */

#include "include/operator_manager.h"
#include "include/database_connection.h"
#include "include/logger.h"
#include "include/config_manager.h"
#include <mariadb/mysql.h>
#include <thread>
#include <chrono>
#include <string>

void OperatorManager::checkInactiveOperators() {
    const ConfigManager& config = ConfigManager::getInstance();
    int checkInterval = 5;      // Sprawdzamy co 5 minut
    int inactiveThreshold = 15; // Uznajemy za nieaktywnego po 15 minutach

    while (true) {
        if (mysql_thread_init() == 0) {
            try {
                DatabaseConnection db;
                if (db.connect(config.getDatabaseHost().c_str(),
                               config.getDatabaseUsername().c_str(),
                               config.getDatabasePassword().c_str(),
                               config.getDatabaseName().c_str())) {

                    std::string sql = "UPDATE operators SET status = 'inactive' "
                                      "WHERE status = 'active' AND last_seen < NOW() - INTERVAL " +
                                      std::to_string(inactiveThreshold) + " MINUTE";

                    MYSQL_RES* res = db.query(sql.c_str());
                    if (res) {
                        mysql_free_result(res);
                    }

                    Logger::getInstance().log("Sprawdzono nieaktywnych operatorów.");
                } else {
                    Logger::getInstance().log("OperatorManager: Nie udalo sie polaczyc z baza danych.", Logger::LogType::Error);
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log("Wyjątek w wątku OperatorManager: " + std::string(e.what()), Logger::LogType::Error);
            }
            mysql_thread_end();
        } else {
            Logger::getInstance().log("mysql_thread_init() failed w watku OperatorManager", Logger::LogType::Error);
        }

        std::this_thread::sleep_for(std::chrono::minutes(checkInterval));
    }
}

