#include "include/daemonizer.h"
#include "include/logger.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib> // Dla exit()
#include <fstream>

const char* Daemonizer::PID_FILE = "/var/run/hamevent.pid";

void Daemonizer::daemonize() {
    pid_t pid, sid;

    // Pierwszy fork
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Zakończ proces rodzicielski
    }

    // Utwórz nową sesję i grupę procesów
    sid = setsid();
    if (sid < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Zmień katalog roboczy
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Zamknij standardowe deskryptory plików
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Przekieruj standardowe strumienie do /dev/null
    open("/dev/null", O_RDWR); // stdin
    open("/dev/null", O_RDWR); // stdout
    open("/dev/null", O_RDWR); // stderr

    // Zapisz PID do pliku
    std::ofstream pidFile(PID_FILE);
    if (pidFile.is_open()) {
        pidFile << getpid() << std::endl;
        pidFile.close();
    } else {
        Logger::getInstance().log("Nie można zapisać PID do pliku: " + std::string(PID_FILE));
    }
}
