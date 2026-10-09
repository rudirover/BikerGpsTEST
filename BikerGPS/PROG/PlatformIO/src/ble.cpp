#include "ble.hpp"
#include "debug.hpp"
#include "sdcard.hpp"

Ble ble;

Ble::Ble() {};

void Ble::init()
{
    if (!bleInitialized)
    {
        BLEDevice::init(BLE_DEVICE_NAME);
        BLEDevice::setMTU(517);

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks());

        pService = pServer->createService(BLE_SERVICE_UUID);
        pCharacteristic = pService->createCharacteristic(
            BLE_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);

        pCharacteristic->setCallbacks(new MyCallbacks());
        pService->start();

        pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
        pAdvertising->setScanResponse(true);

        bleInitialized = true;
    }
}

void Ble::enable()
{
    if (ble.enabled) return;
    if (pAdvertising) {
        pAdvertising->start();
        ble.enabled = true;
    }
}

void Ble::disable()
{
    if (!ble.enabled) return;
    if (pAdvertising) { pAdvertising->stop(); }
    if (deviceConnected && pServer) { pServer->disconnect(pServer->getConnId()); }
    ble.enabled = false;
}

void Ble::MyServerCallbacks::onConnect(BLEServer *pServer)
{
    ble.deviceConnected = true;
    Serial.println("[BLE] Device connected");
}

void Ble::MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    ble.deviceConnected = false;
    Serial.println("[BLE] Device disconnected");
    if (ble.enabled) { BLEDevice::startAdvertising(); }
}

void Ble::pushChunk(BlePacketType type, const std::string &data)
{
    portENTER_CRITICAL(&bleMutex);
    chunkQueue.push_back({type, data});
    portEXIT_CRITICAL(&bleMutex);
}

bool Ble::getNextChunk(BleChunk &chunkOut)
{
    portENTER_CRITICAL(&bleMutex);
    if (chunkQueue.empty()) {
        portEXIT_CRITICAL(&bleMutex);
        return false;
    }
    chunkOut = chunkQueue.front();
    chunkQueue.erase(chunkQueue.begin());
    portEXIT_CRITICAL(&bleMutex);
    return true;
}

void Ble::MyCallbacks::onWrite(BLECharacteristic *pChar)
{
    std::string rxValue = pChar->getValue();
    if (rxValue.empty()) return;

    BlePacketType type = static_cast<BlePacketType>(rxValue[0]);
    std::string payload = rxValue.substr(1);

    ble.pushChunk(type, payload);
}

bool Ble::initDone() { return bleInitialized; }