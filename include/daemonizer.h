#ifndef DAEMONIZER_H
#define DAEMONIZER_H

class Daemonizer {
public:
    static void daemonize();

private:
    static const char* PID_FILE;
    Daemonizer() = delete;
};

#endif // DAEMONIZER_H
