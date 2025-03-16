#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cassert>
#include "ais_bbox_mapper.hpp"
using namespace std;

AisBboxMapper::AisBboxMapper(AisUtil::GeoCoords currentLocation, AisUtil::GeoCoords lookAt)
    : currentLocation(currentLocation), lookAt(lookAt), cameraAngleInRadian(calculateAngleInRadian(currentLocation, lookAt)) {
}

AisBboxMapper::~AisBboxMapper() {
}

void AisBboxMapper::UpdateLocalShipInfo(const vector<AisUtil::ShipInfo>& shipInfoRef, uint32_t width, uint32_t height) {
    localShipInfo.clear();
    localShipScreenCoords.clear();
    const double angle = M_PI_2 - cameraAngleInRadian;
    for (auto it = shipInfoRef.begin(); it != shipInfoRef.end(); it++) {
        // rotate the point around currentLocation based on the camera's orientation
        AisUtil::ShipInfo tmp = *it;
        tmp.geoPos = affineGeoCoords(it->geoPos, currentLocation, angle);
        tmp.cog += float(double(360) * angle / (double(2) * M_PI));
        AisUtil::NormalizeDegree(tmp.cog);
        localShipInfo.push_back(tmp);

        // calculate screen coords
        auto scale = 10.0f;
        auto newX = width / 2 + scale * float(tmp.geoPos.longitude * double(width));
        auto newY = height - scale * float(tmp.geoPos.latitude  * double(height));
        localShipScreenCoords.push_back({ newX, newY });
    }
}

vector<string> AisBboxMapper::SearchForShipName(detectNet::Detection* detections, int numDetections) {
    // convert bounding boxes to points
    vector<AisUtil::ScreenCoords> sDetections;
    for (int i = 0; i < numDetections; i++) {
        auto detection = detections[i];
        auto newX = detection.Left + (detection.Right  - detection.Left) / 2;
        auto newY = detection.Top  + (detection.Bottom - detection.Top)  / 2;
        sDetections.push_back({ newX, newY });
    }

    // calculate distance
    vector<tuple<double, int, int>> distance; // <distance, index of detections, index of localShipInfo>
    for (int i1 = 0; i1 < numDetections; i1++) {
        for (int i2 = 0; i2 < localShipScreenCoords.size(); i2++) {
            auto pos1 = sDetections[i1];
            auto pos2 = localShipScreenCoords[i2];
            auto d = pow(pos2.x - pos1.x, 2) + pow(pos2.y - pos1.y, 2);
            distance.push_back({ d, i1, i2 });
        }
    }
    sort(distance.begin(), distance.end());

#if 0
    int idx = 0;
    for (auto it = distance.begin(); it != distance.end(); it++) {
        cout << idx << ": " << get<0>(*it) << ", " << get<1>(*it) << ", " << get<2>(*it) << endl;
        idx++;
    }
#endif

    // select a candidate localShipInfo for each detection
    vector<string> shipNameList;
    for (int i = 0; i < numDetections; i++) {
        auto it = find_if(distance.begin(), distance.end(), [i](tuple<double, int, int> elm) {
            return get<1>(elm) == i;
        });
        if (it == distance.end()) {
            // no more candidates
            break;
        }

        // push shipName
        auto localShipId = get<2>(*it);
        shipNameList.push_back(localShipInfo[localShipId].shipName);

        // remove selected localShipInfo from distance
        auto it2 = remove_if(distance.begin(), distance.end(), [localShipId](tuple<double, int, int> elm) {
            return get<2>(elm) == localShipId;
        });
        if (it2 != distance.end()) {
            distance.erase(it2, distance.end());
        }
    }
    return shipNameList;
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

const std::vector<AisUtil::ShipInfo>& AisBboxMapper::GetLocalShipInfo(void) {
    return localShipInfo;
}

const vector<AisUtil::ScreenCoords>& AisBboxMapper::GetLocalShipScreenCoords(void) {
    return localShipScreenCoords;
}