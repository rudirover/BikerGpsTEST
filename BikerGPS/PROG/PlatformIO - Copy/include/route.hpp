#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "gps.hpp"

#define SAVED_ROUTE "/saved.json"
#define AUTO_WPT_DIST_METERS 10 // need to be within 10 meters for the waypoints
#define AUTO_TPT_DIST_METERS 30 // need to be within 25 meters for the segments

class Route
{
public:
    Route();
    void init();
    bool exists();
    void save(std::string received);
    void prepare();
    void run();
    bool isValidRouteIndex(int index);
    int activeWptIndex;
    int activeTptIndex;
    int selectedWptIndex;
    int nextWptIndex;
    JsonDocument jsonRoute;
    double getHeading();
    int getWptDistancePct();
    void setRoute();
    bool initDone();
    double targetLat;
    double targetLon;
    double targetDist;
    double travelDist;
private:
    bool startWptReached;
    bool initialized = false;
};

extern Route route;