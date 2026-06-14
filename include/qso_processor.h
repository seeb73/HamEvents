#ifndef QSO_PROCESSOR_H
#define QSO_PROCESSOR_H

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

class QsoProcessor {
public:
    static void processQso(const std::string& receivedData, const struct sockaddr_in& clientAddr);

private:
    QsoProcessor() = delete;
// martwy kod ale kosmetycznie zostaje.
    static std::string join(const std::vector<std::string>& elements, const std::string& separator);
};

#endif // QSO_PROCESSOR_H
