#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <iomanip>
#include "ais_loader.hpp"
#include "logging.h"
using namespace std;
using namespace nlohmann;

class AisPositionReportNotFoundException : exception {
};

AisLoader::AisLoader() {
    isLoaded = false;
}

AisLoader::AisLoader(string ndjsonFileName) {
    isLoaded = LoadAisFromFile(ndjsonFileName);
}

AisLoader::~AisLoader() {
}

bool AisLoader::LoadAisFromFile(string ndjsonFileName) {
    ifstream ifs(ndjsonFileName);
    if (!ifs) {
        LogError("Failed to open the AIS's ndjson file.\n");
        return false;
    }

    // parse NDJSON (newline delimited JSON) file
    string line;
    while (getline(ifs, line)) {
        try {
            const auto jsonData = parseLine(line);
            const auto oneShipInfo = parseJson(jsonData);
            shipInfo.push_back(oneShipInfo);
        } catch (AisPositionReportNotFoundException& e) {
            continue;
        }
    }
    return true;
}

string AisLoader::fixLine(string line) {
    // fix single quoted text to double quoted
    line = regex_replace(line, regex(R"('([^']*?)')"), "\"$1\"");

    // fix boolean value to lower case
    line = regex_replace(line, regex(R"(\bTrue\b)"), "true");
    line = regex_replace(line, regex(R"(\bFalse\b)"), "false");

    return line;
}

json AisLoader::parseLine(string line) {
    line = fixLine(line);
    // LogVerbose("Parsing line: %s\n", line.c_str());
    try {
        json jsonData = json::parse(line);
        return jsonData;
    } catch (const json::parse_error& e) {
        LogError("Failed to parse the AIS's ndjson file.\n");
        throw;
    }
}

AisLoader::ShipInfo AisLoader::parseJson(nlohmann::json jsonData) {
    try {
        const string msgType = jsonData["MessageType"];
        const json msg       = jsonData["Message"];
        const json metaData  = jsonData["MetaData"];
        if (msgType == "PositionReport") {
            // parse time_utc
            string datetimeStr = string(metaData["time_utc"]).substr(0, 19); // ex) "2025-03-12 06:18:25"
            istringstream ss(datetimeStr);
            tm tm = {};
            ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");

            // fill ShipInfo
            ShipInfo shipInfo;
            shipInfo.utcTimeStr = datetimeStr;
            shipInfo.utcTime    = tm;
            shipInfo.shipName   = metaData["ShipName"];
            shipInfo.latitude   = metaData["latitude"];
            shipInfo.longitude  = metaData["longitude"];

            // cout << put_time(&tm, "%Y-%m-%d %H:%M:%S") << " " << shipInfo.shipName << ": " << shipInfo.latitude << "," << shipInfo.longitude << endl;
            return shipInfo;
        } else {
            throw AisPositionReportNotFoundException();
        }
    } catch (const json::parse_error& e) {
        LogError("Failed to access parsed json data.\n");
        throw;
    }
}

bool AisLoader::IsLoaded(void) {
    return isLoaded;
}

size_t AisLoader::GetLoadedEntryCount(void) {
    return shipInfo.size();
}