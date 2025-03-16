#include <iostream>
#include <cmath>
#include "ais_bbox_mapper.hpp"
using namespace std;

AisBboxMapper::AisBboxMapper(AisUtil::GeoCoords currentLocation, AisUtil::GeoCoords lookAt)
    : currentLocation(currentLocation), lookAt(lookAt), cameraAngleInRadian(calculateAngleInRadian(currentLocation, lookAt)) {
}

AisBboxMapper::~AisBboxMapper() {
}

void AisBboxMapper::UpdateLocalShipInfo(const vector<AisUtil::ShipInfo>& shipInfoRef) {
    localShipInfo.clear();
    const double angle = M_PI_2 - cameraAngleInRadian;
    for (auto it = shipInfoRef.begin(); it != shipInfoRef.end(); it++) {
        AisUtil::ShipInfo tmp = *it;
        tmp.geoPos = affineGeoCoords(it->geoPos, currentLocation, angle);
        tmp.cog += float(double(360) * angle / (double(2) * M_PI));
        AisUtil::NormalizeDegree(tmp.cog);
        localShipInfo.push_back(tmp);
    }
}

vector<string> AisBboxMapper::SearchForShipName(detectNet::Detection* detections, int numDetections) {
    vector<AisUtil::ScreenCoords> screenCoordsList;
    for (int i = 0; i < numDetections; i++) {
        auto detection = detections[i];
        screenCoordsList.push_back({ .x = detection.Left + (detection.Right  - detection.Left) / 2,
                                     .y = detection.Top  + (detection.Bottom - detection.Top)  / 2 });
    }
    // TODO: implement matching between screenCoordsList and localShipInfo
}

double AisBboxMapper::calculateAngleInRadian(AisUtil::GeoCoords p0, AisUtil::GeoCoords p) {
    double dx = p.longitude - p0.longitude;
    double dy = p.latitude  - p0.latitude;
    cout << "calculation of camera angle" << endl;
    cout << "dx = " << dx << ", dy = " << dy << endl;
    return atan2(dy, dx);
}

AisUtil::GeoCoords AisBboxMapper::affineGeoCoords(AisUtil::GeoCoords target, AisUtil::GeoCoords origin, double angle) {
    // rotate geoCoords around origin
    double cosVal = cos(angle);
    double sinVal = sin(angle);
    double dx = target.longitude - origin.longitude;
    double dy = target.latitude  - origin.latitude;
    double newX = dx * cosVal - dy * sinVal;
    double newY = dx * sinVal + dy * cosVal;
    return { newY, newX };
}

AisUtil::ScreenCoords AisBboxMapper::calculateScreenCoords(AisUtil::GeoCoords geoCoords) {
    auto offsetLatitude   = geoCoords.latitude  - currentLocation.latitude;
    auto offsetLongtitude = geoCoords.longitude - currentLocation.longitude;
}

void AisBboxMapper::PrintCurrentLocalShipInfo(stringstream* ss) {
    for (auto it = localShipInfo.begin(); it != localShipInfo.end(); it++) {
        AisUtil::PrintShipInfo(*it, ss);
    }
}
void AisBboxMapper::PrintCurrentLocalShipInfoSummary(stringstream* ss) {
    for (auto it = localShipInfo.begin(); it != localShipInfo.end(); it++) {
        AisUtil::PrintShipInfoSummary(*it, ss);
    }
}