#pragma once
#include <vector>
#include <string>
#include <sstream>
#include "detectNet.h"
#include "ais_util.hpp"

class AisBboxMapper {
public:
    AisBboxMapper(AisUtil::GeoCoords currentLocation, AisUtil::GeoCoords lookAt);
    ~AisBboxMapper();
    void UpdateLocalShipInfo(const std::vector<AisUtil::ShipInfo>& shipInfoRef, uint32_t width, uint32_t height);
    std::vector<std::string> SearchForShipName(detectNet::Detection* detections, int numDetections);
    void PrintCurrentLocalShipInfo(std::stringstream* ss=nullptr);
    void PrintCurrentLocalShipInfoSummary(std::stringstream* ss=nullptr);
    const std::vector<AisUtil::ShipInfo>& GetLocalShipInfo(void);
    const std::vector<AisUtil::ScreenCoords>& GetLocalShipScreenCoords(void);
private:
    AisUtil::GeoCoords currentLocation;
    AisUtil::GeoCoords lookAt;
    const double cameraAngleInRadian;
    std::vector<AisUtil::ShipInfo> localShipInfo;
    std::vector<AisUtil::ScreenCoords> localShipScreenCoords;
    double calculateAngleInRadian(AisUtil::GeoCoords p0, AisUtil::GeoCoords p);
    AisUtil::GeoCoords affineGeoCoords(AisUtil::GeoCoords target, AisUtil::GeoCoords origin, double angle);
    AisUtil::ScreenCoords calculateScreenCoords(AisUtil::GeoCoords geoCoords);
};