/**
 * @file qso_processor.cpp
 * @brief Przetwarzanie ramek ADIF z obsługą automatycznej wielo-eventowości i pełnym debugowaniem do pliku logu.
 * @note Wersja automatyczna - uniezależniona od pola my_sig_info (Wariant Wygodny).
 * @version 1.6.2 (Dla HamEvent v0.3.x)
 */

#include "include/qso_processor.h"
#include "include/logger.h"
#include "include/adif_parser.h"
#include "include/xml_pharser.h"
#include "include/config_manager.h"
#include "dx_cluster.h"
#include <mariadb/mysql.h>     // Stabilne C API
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>

static std::string escapeSQL(MYSQL* mysql_handle, const std::string& input) {
    if (!mysql_handle || input.empty()) return "";
    std::vector<char> escaped(input.length() * 2 + 1);
    mysql_real_escape_string(mysql_handle, escaped.data(), input.c_str(), input.length());
    return std::string(escaped.data());
}

static std::string formatDate(const std::string& raw) {
    if (raw.length() != 8) return raw;
    return raw.substr(0, 4) + "-" + raw.substr(4, 2) + "-" + raw.substr(6, 2);
}

static std::string formatTime(const std::string& raw) {
    if (raw.length() < 6) return raw;
    return raw.substr(0, 2) + ":" + raw.substr(2, 2) + ":" + raw.substr(4, 2);
}

void QsoProcessor::processQso(const std::string& rawData, const sockaddr_in& clientAddr) {
    if (mysql_thread_init() != 0) {
        Logger::getInstance().log("mysql_thread_init() failed w watku processQso");
        return;
    }

    std::string clientIP = inet_ntoa(clientAddr.sin_addr);
    auto qsoData = AdifParser::parseRecord(rawData);

	// 2. Jeśli puste, sprawdzamy czy to nie XML (Kolejność ratunkowa)
	if (qsoData.empty()) {
	    XmlParser xmlParser;
	    qsoData = xmlParser.parse(rawData);
	}

// 3. Dopiero teraz sprawdzamy, czy ostatecznie coś wyciągnęliśmy
	if (qsoData.empty()) {
	    Logger::getInstance().log("[PARSER_ERROR] Dane nie są ani poprawnym ADIF, ani XML dla IP: " + clientIP + ". Surowe dane: [" + rawData + "]", Logger::LogType::Error);
	    mysql_thread_end();
	    return;
	}

    bool isDebugMode = true; // Wymuszamy na true, zgodnie z konfiguracją produkcyjną




//if (qsoData.empty()) {
//    Logger::getInstance().log("Dane nie są ani poprawnym ADIF, ani XML dla IP: " + clientIP + ". Surowe dane: [" + rawData + "]", Logger::LogType::Error);
//    mysql_thread_end();
//    return;
//}






/**
    if (qsoData.empty()) {
        Logger::getInstance().log("[PARSER_ERROR] Parser zwrocil pusta mape dla IP: " + clientIP + ". Surowe dane: [" + rawData + "]");
        mysql_thread_end();
        return;
    }
*/
    // PIĘKNY LOG: Zrzut odebranych kluczy w trybie debug
    if (isDebugMode) {
        Logger::getInstance().log("--- TRYB DEBUG: Pakiet od IP [" + clientIP + "] ---");
        for (const auto& [key, val] : qsoData) {
            Logger::getInstance().log("   ADIF/XML Key: [" + key + "] -> Value: [" + val + "]");
        }
    }

    static const std::set<std::string> allowedColumns = {
        "id", "event_id", "station_id", "station_called", "rst_sent", "rst_rcvd",
        "freq", "band", "mode", "name", "qth", "dxcc", "country",
        "cont", "cqz", "ituz", "pfx", "qsl_sent", "lotw_qsl_sent", "tx_pwr",
        "a_index", "band_rx", "clublog_qso_upload_date", "clublog_qso_upload_status",
        "eqsl_qsl_sent", "freq_rx", "k_index", "my_antenna", "my_city", "my_country",
        "my_cq_zone", "my_dxcc", "my_gridsquare", "my_itu_zone", "my_name", "my_rig",
        "operator", "sfi", "station_callsign", "qso_date", "time_on", "qso_date_off", "time_off"
    };

    MYSQL* native_mysql = mysql_init(nullptr);
    if (!native_mysql) {
        Logger::getInstance().log("Blad mysql_init() w watku QSO");
        mysql_thread_end();
        return;
    }

    try {
        const ConfigManager& config = ConfigManager::getInstance();

	std::string db_host = config.getDatabaseHost();
        std::string db_user = config.getDatabaseUsername();
        std::string db_pass = config.getDatabasePassword();
        std::string db_name = config.getDatabaseName();
        unsigned int db_port = 3306;

        if (!mysql_real_connect(native_mysql,
                                db_host.c_str(),
                                db_user.c_str(),
                                db_pass.c_str(),
                                db_name.c_str(),
                                db_port, nullptr, 0)) {
            Logger::getInstance().log("Nie udalo sie polaczyc z baza przez C API: " + std::string(mysql_error(native_mysql)));
            mysql_close(native_mysql);
            mysql_thread_end();
            return;
        }

        // Ustalamy znak operatora - sprawdzamy station_callsign lub operator jako fallback
        std::string activeCallsign = "";
        if (qsoData.count("station_callsign") && !qsoData["station_callsign"].empty()) {
            activeCallsign = qsoData["station_callsign"];
        } else if (qsoData.count("operator") && !qsoData["operator"].empty()) {
            activeCallsign = qsoData["operator"];
        }

        std::vector<std::string> targetEventIds;

        // WARIANT WYGODNY: Zawsze dynamicznie sprawdzamy przypisane aktywne akcje w bazie dla danego znaku
        if (!activeCallsign.empty()) {
            std::string checkSql =
                "SELECT es.event_id FROM event_stations es "
                "JOIN stations s ON es.station_id = s.id "
                "JOIN events e ON es.event_id = e.event_id "
                "WHERE s.callsign = '" + escapeSQL(native_mysql, activeCallsign) + "' "
                "AND es.authorized = 1 "
                "AND NOW() BETWEEN e.event_start AND e.event_end;";

            if (mysql_query(native_mysql, checkSql.c_str()) == 0) {
                MYSQL_RES* resCheck = mysql_store_result(native_mysql);
                if (resCheck) {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(resCheck))) {
                        if (row[0]) targetEventIds.push_back(row[0]);
                    }
                    mysql_free_result(resCheck);
                }
            }
        }

        // Jeśli lista eventów jest pusta (brak autoryzacji lub poza czasem), przerywamy i zamykamy bezpiecznie wątek
        if (targetEventIds.empty()) {
            Logger::getInstance().log("BLAD AUTORYZACJI: IP " + clientIP + " (Stacja: " + activeCallsign + ") wyslala pakiet poza ramami czasowymi wydarzen lub stacja nie jest przypisana.");
            mysql_close(native_mysql);
            mysql_thread_end();
            return;
        }

        if (isDebugMode) {
            Logger::getInstance().log("Liczba docelowych eventow do zapisu: " + std::to_string(targetEventIds.size()));
            Logger::getInstance().log("-----------------------------------------------------------------");
        }

        std::string targetStationId = "1";
        if (qsoData.find("a_index") != qsoData.end()) targetStationId = qsoData["a_index"];
        else if (qsoData.find("A_INDEX") != qsoData.end()) targetStationId = qsoData["A_INDEX"];
        qsoData["station_id"] = targetStationId;

        // Pętla po wszystkich aktywnych i zautoryzowanych eventach dla tej stacji
        for (const std::string& currentEventId : targetEventIds) {
            qsoData["event_id"] = currentEventId;
            std::string sql = "INSERT INTO qso_log (";
            std::string values = "VALUES (";
            bool firstField = true;

            for (auto it = qsoData.begin(); it != qsoData.end(); ++it) {
                std::string columnName = it->first;
                std::string columnValue = it->second;

                std::transform(columnName.begin(), columnName.end(), columnName.begin(), [](unsigned char c) { return std::tolower(c); });
                if (columnName == "call") columnName = "station_called";
                if (columnName == "sig_info" || columnName == "my_sig_info" || columnName == "submode") continue;
                if (allowedColumns.find(columnName) == allowedColumns.end()) continue;

                if (columnName == "qso_date" || columnName == "qso_date_off") columnValue = formatDate(columnValue);
                else if (columnName == "time_on" || columnName == "time_off") columnValue = formatTime(columnValue);

                if (columnName == "freq" && columnValue.empty()) columnValue = "0.0";

                if (!firstField) { sql += ", "; values += ", "; }
                firstField = false;
                sql += "`" + columnName + "`";
                values += "'" + escapeSQL(native_mysql, columnValue) + "'";
            }
            sql += ") " + values + ")";

            if (isDebugMode && !firstField) {
                Logger::getInstance().log("TRYB DEBUG: Wygenerowano SQL dla event_id [" + currentEventId + "]: " + sql);
            }

            if (!firstField) {
                if (mysql_query(native_mysql, sql.c_str()) == 0) {
                    MYSQL_RES* res = mysql_store_result(native_mysql);
                    if (res) mysql_free_result(res);
                } else {
                    Logger::getInstance().log("BŁĄD SQL podczas zapisu dla event_id [" + currentEventId + "]: " + std::string(mysql_error(native_mysql)));
                }
            }
        }

        // DX Cluster Spotting (na koniec, poza pętlą zapisu)
        std::string my_callsign = "";
        std::string hunter_callsign = "";
        std::string freq_str = "";
        std::string band_str = "";

        if (qsoData.count("station_callsign")) my_callsign = qsoData["station_callsign"];
        else if (qsoData.count("operator")) my_callsign = qsoData["operator"];
        if (qsoData.count("call")) hunter_callsign = qsoData["call"];
        else if (qsoData.count("station_called")) hunter_callsign = qsoData["station_called"];
        if (qsoData.count("freq")) freq_str = qsoData["freq"];
        if (qsoData.count("band")) band_str = qsoData["band"];

        if (!my_callsign.empty() && !hunter_callsign.empty() && !freq_str.empty() && freq_str != "0.0") {
            handleDxClusterSpotting(my_callsign, hunter_callsign, freq_str, band_str);
        }

    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Blad podczas przetwarzania QSO: ") + e.what());
    }

    mysql_close(native_mysql);
    mysql_thread_end();
}

