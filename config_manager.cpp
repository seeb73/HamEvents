/**
 * @file config_manager.cpp
 * @brief Implementacja ConfigManager 1.x.
 * @version 1.0.0
 */

#include "include/config_manager.h"
#include "dx_config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[CRITICAL] Nie można otworzyć pliku konfiguracyjnego: " << filename << std::endl;
        throw std::runtime_error("Nie można otworzyć pliku konfiguracyjnego.");
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;

        // Usuwamy komentarze inline
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim spacji i znaków końca linii
            key.erase(key.find_last_not_of(" \t") + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));

            if (!key.empty()) {
                config[key] = value;
            }
        }
    }

    file.close();
    
    // Logowanie sukcesu sparsowania pliku
    std::cout << "[INFO] Centralny plik konfiguracyjny [" << filename << "] został pomyślnie sparsowany." << std::endl;
    std::cout << "[DEBUG] Wczytane klucze bazy -> User: " << getDatabaseUsername() << ", Base: " << getDatabaseName() << std::endl;

    parseDxCluster();
}

int ConfigManager::getUdpPort() const {
    return config.count("udpPort") ? std::stoi(config.at("udpPort")) : 2237;
}

std::string ConfigManager::getDatabaseHost() const {
    if (config.count("dbhost")) return config.at("dbhost");
    return "127.0.0.1";
}

std::string ConfigManager::getDatabaseUsername() const {
    return config.count("dbuser") ? config.at("dbuser") : "hamevents";
}

std::string ConfigManager::getDatabasePassword() const {
    return config.count("dbpassword") ? config.at("dbpassword") : "";
}

std::string ConfigManager::getDatabaseName() const {
    return config.count("dbname") ? config.at("dbname") : "hamevents";
}

std::string ConfigManager::getLogFilePath() const {
    return config.count("log_path") ? config.at("log_path") : "/var/log/hamevent.log";
}

bool ConfigManager::isWrongLogEnabled() const {
    return config.count("wrong_log_enabled") && (config.at("wrong_log_enabled") == "true" || config.at("wrong_log_enabled") == "y");
}

std::string ConfigManager::getWrongLogFilePath() const {
    return config.count("wrong_log_path") ? config.at("wrong_log_path") : "/var/log/hamevent.err";
}

void ConfigManager::parseDxCluster() {
    // Bezpieczne wartości domyślne ustawiane przed parsowaniem
    clusterConfig.dxisEnable = false;
    clusterConfig.dxport = 7300;
    clusterConfig.dxinterval = 15;

    if (config.count("dxisEnable")) {
        std::string val = config.at("dxisEnable");
        clusterConfig.dxisEnable = (val == "true" || val == "1" || val == "y");
    }
    
    if (config.count("dxserver")) {
        clusterConfig.dxserver = config.at("dxserver");
    }
    
    if (config.count("dxport")) {
        clusterConfig.dxport = std::stoi(config.at("dxport"));
    }
    
    if (config.count("dxuser")) {
        clusterConfig.dxuser = config.at("dxuser");
    }
    
    if (config.count("dxinterval")) {
        clusterConfig.dxinterval = std::stoi(config.at("dxinterval"));
    }
}

// obsługujemy globalnie debugowanie z configa z bezpiecznikiem na bzdury powinno być true/false ale komuś może wpaść do głowy pomysł 1/0
bool ConfigManager::isDebug() const {
    return config.count("isDebug") && (config.at("isDebug") == "true" || config.at("isDebug") == "1");
}
