#pragma once
#include <iomanip>
#include <cstdint>

class AisUtil {
public:
    typedef struct {
        double latitude;  // [degree]
        double longitude; // [degree]
    } GeoCoords;
    typedef struct {
        float x; // [pixel]
        float y; // [pixel]
    } ScreenCoords;
    typedef struct {
        std::time_t unixTime;
        uint32_t mmsi;    // vessel id
        std::string shipName;
        GeoCoords geoPos; // position
        float cog;        // course over ground [degree]
        float sog;        // speed over ground [knot]
    } ShipInfo;
    static void PrintShipInfo(ShipInfo& shipInfo);
    static void NormalizeDegree(float& degree);
    static void NormalizeRadian(double& rad);
};