#pragma once

class AisUtil {
public:
    typedef struct {
        double latitude;  // [degree]
        double longitude; // [degree]
    } GeoCoords;
    typedef struct {
        int x; // [pixel]
        int y; // [pixel]
    } ScreenCoords;
    typedef struct {
        std::time_t unixTime;
        std::string shipName;
        GeoCoords geoPos; // position
        float cog;        // course over groupd [degree]
        float sog;        // speed over ground [knot]
    } ShipInfo;
};