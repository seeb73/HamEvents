#include "include/udp_server.h"
#include "include/logger.h"
#include "include/qso_processor.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <string>
#include <cstring> // For memset
// bezpieczne debugowanie suurowych pakietów... 
#include "include/config_manager.h"
#include <iomanip>
#include <chrono>


UdpServer::UdpServer(int serverPort) : port(serverPort), socketFd(-1) {
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        perror("socket creation failed");
        throw std::runtime_error("Nie można utworzyć gniazda UDP.");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(socketFd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind failed");
        close(socketFd);
        throw std::runtime_error("Nie można powiązać gniazda UDP.");
    }

    Logger::getInstance().log("Serwer UDP nasłuchuje na porcie " + std::to_string(port) + "...");
    std::cout << "Serwer UDP nasłuchuje na porcie " << port << "..." << std::endl;
}

UdpServer::~UdpServer() {
    if (socketFd != -1) {
        close(socketFd);
    }
}

void UdpServer::run() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    while (true) {
        ssize_t recvLen = recvfrom(socketFd, buffer, BUFFER_SIZE, 0,
                                   (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (recvLen < 0) {
            perror("recvfrom failed");
            Logger::getInstance().log("Błąd podczas odbierania danych UDP: " + std::string(strerror(errno)));
            continue;
        }
	if (ConfigManager::getInstance().isDebug()) {
    	  std::ofstream rawLogFile("/var/log/hamevents/hamevents.raw", std::ios::app);
    	  if (rawLogFile.is_open()) {
        	auto now = std::chrono::system_clock::now();
        	auto in_time_t = std::chrono::system_clock::to_time_t(now);

        	char ipStr[INET_ADDRSTRLEN];
        	inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

        	rawLogFile << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "] "
                   << "[" << ipStr << "] "
                   << std::string(buffer, recvLen) << "\n";
        	rawLogFile.close();
    	}
      }


        std::thread qsoThread(QsoProcessor::processQso, std::string(buffer, recvLen), clientAddr);
        qsoThread.detach();
    }
}
