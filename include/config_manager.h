#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>

class ConfigManager {
public:
    static ConfigManager& getInstance();
    void loadConfig(const std::string& filename);

    // Port serwera UDP
    int getUdpPort() const;

    std::string getDatabaseHost() const;
    std::string getDatabaseUsername() const;
    std::string getDatabasePassword() const;
    std::string getDatabaseName() const;

    std::string getLogFilePath() const;
    bool isWrongLogEnabled() const;
    std::string getWrongLogFilePath() const;

   bool isDebug() const;

private:
    ConfigManager() = default;
    std::map<std::string, std::string> config;
    
    // Prywatna metoda pomocnicza do parsowania sekcji [dxcluster]
    void parseDxCluster();
};

#endif // CONFIG_MANAGER_H
