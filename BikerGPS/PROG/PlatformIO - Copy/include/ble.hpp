#pragma once

#include "debug.hpp"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define BLE_DEVICE_NAME "BikerNetworkTool"
#define BLE_SERVICE_UUID "0000180f-0000-1000-8000-00805f9b34fb"
#define BLE_CHARACTERISTIC_UUID "00002a19-0000-1000-8000-00805f9b34fb"

class Ble
{
public:
    Ble();
    void init();
    void enable();
    void disable();
    bool enabled;
    bool routeAvailable;
    bool deviceConnected = false;
    std::string receivedRoute;
    std::string receivedFilename; // Stores parsed filename
    bool initDone();

private:
    BLEServer *pServer;
    BLEService *pService;
    BLEAdvertising *pAdvertising;
    BLECharacteristic *pCharacteristic;
    bool bleInitialized = false;

    class MyServerCallbacks : public BLEServerCallbacks
    {
    public:
        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
    };
    class MyCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pChar) override;
    };
};

extern Ble ble;