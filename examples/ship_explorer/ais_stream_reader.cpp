#include <iostream>
#include "ais_stream_reader.hpp"
using namespace std;

AisStreamReader::AisStreamReader(const vector<AisUtil::ShipInfo>& shipInfo) : originalShipInfo(shipInfo) {
    cursor = 0;
    lastUpdatedUnixTime = 0;
    timeWindowSize = 60 * 3;
}

AisStreamReader::~AisStreamReader() {
}

void AisStreamReader::Update(time_t limitUnixTime) {
    // remove existing entries outside the time window
    assert(lastUpdatedUnixTime <= limitUnixTime);
    assert(timeWindowSize <= limitUnixTime);
    auto startUnixTime = limitUnixTime - timeWindowSize;
    auto it = remove_if(currentShipInfo.begin(), currentShipInfo.end(), [startUnixTime](const AisUtil::ShipInfo& tmp) {
        return tmp.unixTime < startUnixTime;
    });
    if (it != currentShipInfo.end()) {
        currentShipInfo.erase(it, currentShipInfo.end());
    }

    // append new entries
    for (auto curShipIt = originalShipInfo.begin() + cursor;
        curShipIt != originalShipInfo.end() && curShipIt->unixTime <= limitUnixTime;
        cursor++, curShipIt++) {

        // skip if the entry is outside the time window
        if (curShipIt->unixTime < startUnixTime) {
            continue;
        }

        // remove the existing entry with the same MMSI (will be found up to 1 entry)
        auto curMmsi = curShipIt->mmsi;
        auto it = find_if(currentShipInfo.begin(), currentShipInfo.end(), [curMmsi](const AisUtil::ShipInfo& tmp) {
            return tmp.mmsi == curMmsi;
        });
        if (it != currentShipInfo.end()) {
            currentShipInfo.erase(it);
        }

        // push new entry
        currentShipInfo.push_back(*curShipIt);
    }

    lastUpdatedUnixTime = limitUnixTime;
}

const vector<AisUtil::ShipInfo>& AisStreamReader::GetCurrentWindow(void) {
    return currentShipInfo;
}

void AisStreamReader::PrintCurrentWindow(void) {
    for (auto it = currentShipInfo.begin(); it != currentShipInfo.end(); it++) {
        AisUtil::PrintShipInfo(*it);
    }
}