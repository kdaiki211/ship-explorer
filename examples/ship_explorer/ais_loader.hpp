#pragma once

#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include "json.hpp"
#include "ais_util.hpp"

class AisLoader {
public:
    AisLoader();
    AisLoader(std::string ndjsonFileName);
    ~AisLoader();
    bool LoadAisFromFile(std::string ndjsonFileName);
    bool IsLoaded(void);
    size_t GetLoadedEntryCount(void);
    const std::vector<AisUtil::ShipInfo>& GetLoadedShipInfo(void);
    static void PrintShipInfo(AisUtil::ShipInfo& shipInfo);

private:
    std::string ndjsonFileName;
    bool isLoaded;
    std::vector<AisUtil::ShipInfo> shipInfo;

    std::string fixLine(std::string line);
    nlohmann::json parseLine(std::string line);
    AisUtil::ShipInfo parseJson(nlohmann::json jsonData);
};
