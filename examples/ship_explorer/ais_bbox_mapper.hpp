#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <opencv2/opencv.hpp>
#include "detectNet.h"
#include "ais_util.hpp"

class AisBboxMapper {
public:
    AisBboxMapper(const AisUtil::GeoCoords srcGeoPoints[], const cv::Point2f dstScreenPoints[]);
    ~AisBboxMapper();
    void UpdateLocalShipInfo(time_t unixTime, const std::vector<AisUtil::ShipInfo>& shipInfoRef);
    std::vector<std::string> SearchForShipName(detectNet::Detection* detections, int numDetections, float distanceThreshold);
    void PrintCurrentLocalShipInfo(std::stringstream* ss=nullptr);
    void PrintCurrentLocalShipInfoSummary(std::stringstream* ss=nullptr);
    const std::vector<AisUtil::ShipInfo>& GetLocalShipInfo(void);
    const std::vector<AisUtil::ScreenCoords>& GetLocalShipScreenCoords(void);
    const std::vector<AisUtil::ScreenCoords>& GetDetectedPoint(void);
    AisUtil::ScreenCoords ConvertGeoCoordsToScreenCoords(const AisUtil::GeoCoords& geoPos);

private:
    cv::Mat perspectiveMatrix;
    std::vector<AisUtil::ShipInfo> localShipInfo;
    std::vector<AisUtil::ScreenCoords> localShipScreenCoords;
    std::vector<AisUtil::ScreenCoords> detectedPoint;
    static AisUtil::GeoCoords predictCurrentPosition(AisUtil::GeoCoords pos, double sog, double cog, double n);
    cv::Mat calculatePerspectiveMatrix(const AisUtil::GeoCoords srcGeoPoints[], const cv::Point2f dstScreenPoints[]);
};