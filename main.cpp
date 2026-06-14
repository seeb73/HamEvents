#include "include/config_manager.h"
#include "include/logger.h"
#include "include/udp_server.h"
#include "include/operator_manager.h"
#include "include/daemonizer.h"
#include <thread>
#include <iostream>

int main() {
    // 1. Inicjalizacja i wymuszenie wczytania centralnego pliku konfiguracyjnego
    ConfigManager& config = ConfigManager::getInstance();
    try {
        config.loadConfig("/etc/hamevents/hamevents.conf");
    } catch (const std::runtime_error& e) {
        std::cerr << "[CRITICAL] Start przerwany: " << e.what() << std::endl;
        return 1; // Kończymy pracę, jeśli nie ma configu
    }

    const int udpPort = config.getUdpPort();

    // 2. Demonizacja procesu
    Daemonizer::daemonize();
	Logger::getInstance().log("Aplikacja uruchomiona jako daemon.");
	Logger::getInstance().log("=================================================================");
	Logger::getInstance().log("   HamEvent Daemon v0.3.1 - Server Successfully Started");
	Logger::getInstance().log("   Build Date: " + std::string(__DATE__) + " " + std::string(__TIME__));
	Logger::getInstance().log("   Database Driver: MariaDB Native C API (Thread-Safe Mode)");
	Logger::getInstance().log("   Listening for WSJT-X / UDP XML/ADIF packets...");
	Logger::getInstance().log("=================================================================");

    // 3. Uruchomienie wątku sprawdzania nieaktywnych operatorów
    std::thread inactiveCheckThread(OperatorManager::checkInactiveOperators);
    inactiveCheckThread.detach();

    // 4. Start serwera UDP
    UdpServer udpServer(udpPort);
    udpServer.run();

    return 0;
}
