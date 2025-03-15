#pragma once

class AisUtil {
public:
    typedef struct {
        double latitude;
        double longitude;
    } GeoCoords;
    typedef struct {
        std::time_t unixTime;
        std::string shipName;
        GeoCoords geoPos;
    } ShipInfo;
};