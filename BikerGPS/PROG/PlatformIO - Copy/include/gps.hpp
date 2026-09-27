#pragma once

#include "debug.hpp"
#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "buzzer.hpp"

//#define SIMULATION 

#define GPS_RX_PIN 17
#define GPS_TX_PIN 18
#define GPS_ENABLE_PIN 2
#define GPS_BAUD_RATE 9600
#define GPS_TICKTIME 1000
#define MIN_VALID_COG_SPEED 3

class Gps
{
public:
    Gps();
    void init();
    void run();
    void sleep();    
    double latitude;
    double longitude;
    double speedKmph;
    double cogDegrees;
    double bearing(double targetLat, double targetLon);
    double distance(double targetLat, double targetLon);
    bool satellitesIsAvailable();
    bool locationIsAvailable();
    bool courseIsAvailable();
    bool speedIsAvailable();
    bool cogIsAvailable();

private:
#ifndef SIMULATION
    HardwareSerial gpsComm;
#endif
    TinyGPSPlus tinyGps;
    bool cogValid = false;
    unsigned long tickTime = 0;
};

extern Gps gps;