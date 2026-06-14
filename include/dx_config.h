#ifndef DX_CONFIG_H
#define DX_CONFIG_H

#include <string>

struct DxClusterConfig {
    bool dxisEnable = false;
    std::string dxserver = "dxcluster.pl";
    int dxport = 7300;
    std::string dxuser = "";        // Jeśli puste, system podstawi znak korespondenta
    int dxinterval = 15;            // Czas blokady kolejnego spota w minutach
};

// Globalna zmienna konfiguracyjna (dostępna w całym programie)
extern DxClusterConfig clusterConfig;

#endif // DX_CONFIG_H
