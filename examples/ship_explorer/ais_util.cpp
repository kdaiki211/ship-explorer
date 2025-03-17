#include <iostream>
#include <iomanip>
#include <cmath>
#include "ais_util.hpp"
using namespace std;

void AisUtil::PrintShipInfo(AisUtil::ShipInfo& shipInfo, stringstream* ss) {
    tm tm;
    localtime_r(&shipInfo.unixTime, &tm);

    // convert UTC to JST considering carry up
    tm.tm_hour += 9;
    NormalizeTm(tm);

    ostream* os = &cout;
    if (ss != nullptr) {
        os = ss;
    }
    *os << put_time(&tm, "%Y-%m-%d %H:%M:%S ") << "[" << shipInfo.mmsi << "] " << setw(20) << left << shipInfo.shipName << ", (" << shipInfo.geoPos.latitude << "," << shipInfo.geoPos.longitude << "), " << shipInfo.sog << " knots, " << shipInfo.cog << " degs";
}

void AisUtil::PrintShipInfoSummary(AisUtil::ShipInfo& shipInfo, stringstream* ss) {
    tm tm;
    localtime_r(&shipInfo.unixTime, &tm);

    // convert UTC to JST considering carry up
    tm.tm_hour += 9;
    NormalizeTm(tm);

    ostream* os = &cout;
    if (ss != nullptr) {
        os = ss;
    }
    *os << put_time(&tm, "%H:%M:%S ") << setw(20) << left << shipInfo.shipName << " @ (" << shipInfo.geoPos.latitude << "," << shipInfo.geoPos.longitude << "), " << shipInfo.sog << " knots, " << shipInfo.cog << " degs";
}

void AisUtil::NormalizeTm(tm& tm) {
    auto unixTime = mktime(&tm);
    localtime_r(&unixTime, &tm);
}

void AisUtil::NormalizeDegree(float& degree) {
    const auto th = float(360);
    while (degree >= th) {
        degree -= th;
    }
    while (degree <= -th) {
        degree += th;
    }
}

void AisUtil::NormalizeRadian(double& rad) {
    const auto th = double(2) * M_PI;
    while (rad >= th) {
        rad -= th;
    }
    while (rad <= -th) {
        rad += th;
    }
}

void AisUtil::AddSeconds(tm& tm, int seconds) {
    auto unixTime = mktime(&tm) + seconds;
    localtime_r(&unixTime, &tm);
}

std::string AisUtil::RTrim(const std::string& str) {
    size_t end = str.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}