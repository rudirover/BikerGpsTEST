#pragma once

#include "debug.hpp"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <FS.h>
#include "sdcard.hpp"
#include <vector>

#define BLE_DEVICE_NAME "BikerNetworkTool"
#define BLE_SERVICE_UUID "0000180f-0000-1000-8000-00805f9b34fb"
#define BLE_CHARACTERISTIC_UUID "00002a19-0000-1000-8000-00805f9b34fb"


enum class BlePacketType : uint8_t {
    START_FILE = 0x01,
    DATA_CHUNK = 0x02,
    END_FILE   = 0x03
};

struct BleChunk {
    BlePacketType type;
    std::string data; // Contains filename on START_FILE, raw bytes on DATA_CHUNK
};

class Ble {
public:
    Ble();
    void init();
    void enable();
    void disable();
    bool initDone();

    void pushChunk(BlePacketType type, const std::string &data);
    bool getNextChunk(BleChunk &chunkOut);

    bool deviceConnected = false;
    bool enabled = false;
    std::string receivedFilename;

private:
    bool bleInitialized = false;
    BLEServer *pServer = nullptr;
    BLEService *pService = nullptr;
    BLECharacteristic *pCharacteristic = nullptr;
    BLEAdvertising *pAdvertising = nullptr;

    std::vector<BleChunk> chunkQueue;
    portMUX_TYPE bleMutex = portMUX_INITIALIZER_UNLOCKED;

    class MyServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
    };

    class MyCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pChar) override;
    };
};

extern Ble ble;