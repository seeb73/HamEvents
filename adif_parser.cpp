/**
 * @file adif_parser.cpp
 * @brief Przywrócona, stabilna wersja parsera ADIF oparta na regex ze starego serwera cpp.
 * @note Wzbogacona o ujednolicanie wielkości liter kluczy (tolower) dla kompatybilności.
 * @version 1.0.2
 */

#include "include/adif_parser.h"
#include <regex>
#include <algorithm>
#include <cctype>

// Funkcja pomocnicza do usuwania znaków \n, \r oraz spacji z początku i końca stringa
std::string trimField(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();

    return (start < end) ? std::string(start, end) : "";
}

std::map<std::string, std::string> AdifParser::parseRecord(const std::string& adifRecord) {
    std::map<std::string, std::string> result;
    std::regex fieldRegex("<(\\w+):\\d+>([^<]+)");
    std::smatch match;
    std::string data = adifRecord;

    while (std::regex_search(data, match, fieldRegex)) {
        if (match.size() == 3) {
            std::string key = match[1];
            
            // Ujednolicamy klucz do małych liter, aby uniknąć problemów w qso_processor
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

            // Oczyszczamy wartość przed zapisaniem jej do mapy
            std::string value = trimField(match[2]);

            result[key] = value;
        }
        data = match.suffix();
    }
    return result;
}

