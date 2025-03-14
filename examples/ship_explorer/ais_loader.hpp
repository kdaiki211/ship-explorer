#include <string>
#include <vector>
#include <iomanip>
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
        std::string utcTimeStr;
        std::tm utcTime;
        std::string shipName;
        double latitude;
        double longitude;
    } ShipInfo;

private:
    std::string ndjsonFileName;
    bool isLoaded;
    std::vector<ShipInfo> shipInfo;

    std::string fixLine(std::string line);
    nlohmann::json parseLine(std::string line);
    ShipInfo parseJson(nlohmann::json jsonData);
};
