#pragma once

#include <vector>
#include <iomanip>
#include <ctime>
#include "ais_util.hpp"
#include "ais_loader.hpp"

class AisStreamReader {
public:
    AisStreamReader(const std::vector<AisUtil::ShipInfo>& shipInfo);
    ~AisStreamReader();
    void Update(std::time_t limitUnixTime);
    const std::vector<AisUtil::ShipInfo>& GetCurrentWindow(void);
    void PrintCurrentWindow(void);
private:
    std::vector<AisUtil::ShipInfo> originalShipInfo;
    std::vector<AisUtil::ShipInfo> currentShipInfo;
    time_t timeWindowSize;
    time_t lastUpdatedUnixTime;
    unsigned int cursor;
};