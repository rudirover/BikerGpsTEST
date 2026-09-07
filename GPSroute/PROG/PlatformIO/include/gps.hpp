#pragma once

#include "debug.hpp"
#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "buzzer.hpp"

#define SIMULATION 

#define GPS_RX_PIN 43
#define GPS_TX_PIN 44
#define GPS_ENABLE_PIN 2
#define GPS_BAUD_RATE 9600
#define GPS_TICKTIME 1000
#define GPS_ENABLE_PIN 2
#define MIN_VALID_COG_SPEED 3

class Gps
{
public:
    Gps();
    void init();
    void run();
    bool hasValidLocation;
    bool hasValidCog;
    double latitude;
    double longitude;
    double speedKmph;
    double cogDegrees;
    double bearing(double targetLat, double targetLon);
    double distance(double targetLat, double targetLon);
    bool satelliteFix();
    bool initDone();
    void sleep();

private:
#ifndef SIMULATION
    HardwareSerial gpsComm;
#endif
    TinyGPSPlus tinyGps;
    unsigned long tickTime = 0;
    bool initialized = false;
};

extern Gps gps;