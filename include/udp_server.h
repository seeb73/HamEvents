#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

class UdpServer {
public:
    UdpServer(int port);
    ~UdpServer();
    void run();

private:
    int port;
    int socketFd;
    static const int BUFFER_SIZE = 65535;
    void handleIncomingData(int clientSocket, char* buffer, ssize_t recvLen, struct sockaddr_in clientAddr, socklen_t clientAddrLen);
};

#endif // UDP_SERVER_H
