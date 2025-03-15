#pragma once

#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include "json.hpp"

class AisLoader {
public:
    AisLoader();
    AisLoader(std::string ndjsonFileName);
    ~AisLoader();
    bool LoadAisFromFile(std::string ndjsonFileName);
    bool IsLoaded(void);
    size_t GetLoadedEntryCount(void);
    typedef struct {
        std::time_t unixTime;
        std::string shipName;
        double latitude;
        double longitude;
    } ShipInfo;
    std::vector<ShipInfo> GetLoadedShipInfo(void);
    static void PrintShipInfo(ShipInfo& shipInfo);

private:
    std::string ndjsonFileName;
    bool isLoaded;
    std::vector<ShipInfo> shipInfo;

    std::string fixLine(std::string line);
    nlohmann::json parseLine(std::string line);
    ShipInfo parseJson(nlohmann::json jsonData);
};
