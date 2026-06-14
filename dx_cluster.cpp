/**
 * @file dx_cluster.cpp
 * @brief Automatyczne rozgłaszanie aktywności na DX Cluster (Reverse-Spotting / Skimmer).
 * System loguje się jako korespondent (łowca) i wysyła spota dla stacji okolicznościowej.
 * Kolejne spoty na danym paśmie są blokowane na czas interwału zdefiniowanego w hamevents.conf.
 * @note Wersja zautomatyzowana (Wariant Wygodny). Posiada pełne zabezpieczenie wątków (mutex) 
 * oraz zaimplementowany tryb testowy omijający blokadę anty-spamową.
 * @version 1.1.2 (Dla HamEvent v0.3.x)
 */


#include "dx_cluster.h"
#include "dx_config.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <chrono>
#include <thread>
#include <mutex> // Dodane dla pełnego bezpieczeństwa wątków

// Definicja zmiennej globalnej z konfiguracją
DxClusterConfig clusterConfig;

// Struktura klucza do mapy: unikalna para (nasz znak + pasmo)
struct LastSpotKey {
    std::string my_callsign;
    std::string band;

    bool operator<(const LastSpotKey& other) const {
        if (my_callsign != other.my_callsign) return my_callsign < other.my_callsign;
        return band < other.band;
    }
};

// Mapa przechowująca punkt w czasie ostatniego wysłanego spota oraz mutex do jej ochrony
static std::map<LastSpotKey, std::chrono::steady_clock::time_point> lastSpots;
static std::mutex spotsMutex;

// Prywatna funkcja realizująca surowe połączenie TCP Telnet
static void sendDxSpotSocket(const std::string& freq_mhz, const std::string& dx_callsign, const std::string& spotter_callsign) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[DX_CLUSTER] Blad tworzenia socketu TCP\n";
        return;
    }

    // DNS lookup dla serwera cluster
    struct hostent* he = gethostbyname(clusterConfig.dxserver.c_str());
    if (!he) {
        std::cerr << "[DX_CLUSTER] Nie mozna rozwiazac hosta: " << clusterConfig.dxserver << "\n";
        close(sock);
        return;
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(clusterConfig.dxport);
    std::memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    // Timeout (3 sekundy), zeby watek nie wisial w nieskonczonosc przy awarii sieci
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[DX_CLUSTER] Polaczenie z " << clusterConfig.dxserver << " nieudane\n";
        close(sock);
        return;
    }

    // Jeśli w configu nie ma zdefiniowanego stałego usera, logujemy się jako KOORESPONDENT (spotter_callsign)
    // Dzięki temu klaster uzna go za autora tego spota.
    std::string login_user = clusterConfig.dxuser.empty() ? spotter_callsign : clusterConfig.dxuser;
    if (login_user.empty()) {
        login_user = "ANONYMOUS";
    }

    usleep(200000); // 200ms na zgloszenie sie clustera

    // 1. Logowanie do clustera
    std::string login_cmd = login_user + "\r\n";
    send(sock, login_cmd.c_str(), login_cmd.length(), 0);
    usleep(300000); // 300ms na przetworzenie logowania

    // 2. Formatowanie i wyslanie komendy spota
    // dx_callsign to teraz nasz znak okolicznościowy (cel spota)
    std::string comment = "HamEvents_Live_Alert";
    std::string spot_cmd = "DX " + freq_mhz + " " + dx_callsign + " " + comment + "\r\n";

    send(sock, spot_cmd.c_str(), spot_cmd.length(), 0);
    std::cout << "[DX_CLUSTER] Spot wyslany pomyslnie: " << spot_cmd;

    usleep(100000);
    close(sock);
}

void handleDxClusterSpotting(const std::string& my_callsign, const std::string& hunter_callsign, const std::string& freq, const std::string& band) {

    if (!clusterConfig.dxisEnable) {
        return;
    }

    // SP56POZNAN BACKDOOR: Jesli interwal to milion, a wyspotowanym znakiem (lowca) jest SP56POZNAN, omijamy blokade antyspamowa
    bool isDevTestMode = (clusterConfig.dxinterval == 1000000 && hunter_callsign == "SP56POZNAN");

    if (!isDevTestMode) {
        auto now = std::chrono::steady_clock::now();
        LastSpotKey key{my_callsign, band};

        // Zabezpieczamy dostęp do mapy przed jednoczesnym zapisem z wielu wątków
        std::lock_guard<std::mutex> lock(spotsMutex);

        // Sprawdzenie interwalu czasowego
        if (lastSpots.find(key) != lastSpots.end()) {
            auto last_time = lastSpots[key];
            auto elapsed_minutes = std::chrono::duration_cast<std::chrono::minutes>(now - last_time).count();

            if (elapsed_minutes < clusterConfig.dxinterval) {
                // Blokada czasowa aktywna – ignorujemy pakiet
                std::cout << "[DX_CLUSTER] [DEBUG] Pasmo " << band << " zablokowane (anty-spam). Pozostalo minut: "
                          << (clusterConfig.dxinterval - elapsed_minutes) << std::endl;
                return;
            }
        }
        // Zapisujemy czas udanej wysylki spota dla tego pasma
        lastSpots[key] = now;
    } else {
        std::cout << "[DX_CLUSTER] [DEBUG] AKTYWNY TRYB TESTOWY (SP56POZNAN). Omijanie blokady czasowej!" << std::endl;
    }

    std::string freq_mhz = freq;

    // Odpalamy wysylanie w tle za pomoca asynchronicznego watku
    // ZAMIANA RÓL: Cel spota (dx_callsign) = my_callsign, Autor spota (spotter) = hunter_callsign
    std::thread([freq_mhz, my_callsign, hunter_callsign]() {
        sendDxSpotSocket(freq_mhz, my_callsign, hunter_callsign);
    }).detach();
}
