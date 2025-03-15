#pragma once
#include <vector>
#include <string>
#include "detectNet.h"
#include "ais_util.hpp"

class AisBboxMapper {
public:
    AisBboxMapper(AisUtil::GeoCoords currentLocation, AisUtil::GeoCoords lookAt);
    ~AisBboxMapper();
    void UpdateLocalShipInfo(const std::vector<AisUtil::ShipInfo>& shipInfoRef);
    std::vector<std::string> SearchForShipName(detectNet::Detection* detections, int numDetections);
    void PrintCurrentLocalShipInfo(void);
private:
    AisUtil::GeoCoords currentLocation;
    AisUtil::GeoCoords lookAt;
    const double cameraAngleInRadian;
    std::vector<AisUtil::ShipInfo> localShipInfo;
    double calculateAngleInRadian(AisUtil::GeoCoords p0, AisUtil::GeoCoords p);
    AisUtil::GeoCoords affineGeoCoords(AisUtil::GeoCoords target, AisUtil::GeoCoords origin, double angle);
    AisUtil::ScreenCoords calculateScreenCoords(AisUtil::GeoCoords geoCoords);
};