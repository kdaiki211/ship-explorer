#pragma once

#include <vector>
#include <deque>
#include <iomanip>
#include <ctime>
#include "ais_loader.hpp"

class AisStreamReader {
public:
    AisStreamReader(std::vector<AisLoader::ShipInfo> shipInfo);
    ~AisStreamReader();
    void Update(std::time_t limitUnixTime);
    std::deque<AisLoader::ShipInfo> GetCurrentWindow(void);
    void PrintCurrentWindow(void);
private:
    std::vector<AisLoader::ShipInfo> originalShipInfo;
    std::deque<AisLoader::ShipInfo> currentShipInfo;
    time_t timeWindowSize;
    time_t lastUpdatedUnixTime;
    unsigned int cursor;
};