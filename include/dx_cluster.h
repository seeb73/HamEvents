#ifndef DX_CLUSTER_H
#define DX_CLUSTER_H

#include <string>

// Główna funkcja logiki (sprawdza czas i decyduje o wysłaniu spota)
void handleDxClusterSpotting(const std::string& my_callsign, 
                             const std::string& hunter_callsign, 
                             const std::string& freq, 
                             const std::string& band);

#endif // DX_CLUSTER_H
