#include "ble.hpp"

Ble ble;

Ble::Ble() {};

void Ble::init()
{
    if (!bleInitialized)
    {
        DBG_EXT(DBG_INFO, "===== BLE =====");
        DBG_EXT(DBG_INFO, "First time initialization");
        DBG_EXT(DBG_INFO, "Creating stack");
        try
        {
            BLEDevice::init(BLE_DEVICE_NAME);
            
            // Allow the BLE stack to negotiate up to 517 bytes MTU
            BLEDevice::setMTU(517);
            DBG_EXT(DBG_INFO, "Device initialized with max MTU support");

            pServer = BLEDevice::createServer();
            pServer->setCallbacks(new MyServerCallbacks());
            DBG_EXT(DBG_INFO, "Server created");

            pService = pServer->createService(BLE_SERVICE_UUID);
            DBG_EXT(DBG_INFO, "Service created");

            pCharacteristic = pService->createCharacteristic(
                BLE_CHARACTERISTIC_UUID,
                BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);

            pCharacteristic->setCallbacks(new MyCallbacks());
            pService->start();
            DBG_EXT(DBG_INFO, "Characteristic created");

            pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
            pAdvertising->setScanResponse(true);
            DBG_EXT(DBG_INFO, "Advertising service");

            bleInitialized = true;
        }
        catch (...)
        {
            DBG_EXT(DBG_ERROR, "Failed to initialize stack");
            bleInitialized = false;
            return;
        }
    }
}

void Ble::enable()
{
    ble.routeAvailable = false;
    if (ble.enabled)
    {
        DBG_EXT(DBG_INFO, "Already enabled, skipping");
        return;
    }

    DBG_EXT(DBG_INFO, "Enabling BLE");

    if (!bleInitialized)
    {
        DBG_EXT(DBG_WARNING, "Stack not initialized");
    }
    else
    {
        DBG_EXT(DBG_INFO, "Reusing stack, restarting advertising");
        if (pAdvertising && deviceConnected)
        {
            delay(50);
        }
    }

    if (pAdvertising)
    {
        try
        {
            pAdvertising->start();
            ble.enabled = true;
            String address = String(BLEDevice::getAddress().toString().c_str());
            DBG_EXT(DBG_INFO, "Advertising started");
            DBG_EXT(DBG_INFO, "Address: %s", address.c_str());
        }
        catch (...)
        {
            DBG_EXT(DBG_ERROR, "Failed to start advertising");
            ble.enabled = false;
        }
    }
    else
    {
        DBG_EXT(DBG_ERROR, "pAdvertising is null");
    }
}

void Ble::disable()
{
    if (!ble.enabled)
    {
        Serial.println("[BLE] Already disabled");
        return;
    }

    Serial.println("[BLE] Disabling BLE advertising...");

    if (pAdvertising)
    {
        try
        {
            pAdvertising->stop();
        }
        catch (...) {}
    }

    if (deviceConnected && pServer)
    {
        try
        {
            pServer->disconnect(pServer->getConnId());
        }
        catch (...) {}
    }
    ble.enabled = false;
    Serial.println("[BLE] BLE disabled");
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
    if (ble.enabled)
    {
        BLEDevice::startAdvertising();
    }
}

void Ble::MyCallbacks::onWrite(BLECharacteristic *pChar)
{
    std::string rxValue = pChar->getValue();

    if (rxValue.size() < sizeof(uint32_t)) {
        DBG_EXT(DBG_ERROR, "[BLE] Received packet too small");
        return;
    }

    // 1. Extract filename length (first 4 bytes)
    uint32_t filenameLen = 0;
    memcpy(&filenameLen, rxValue.data(), sizeof(uint32_t));

    if (rxValue.size() < sizeof(uint32_t) + filenameLen) {
        DBG_EXT(DBG_ERROR, "[BLE] Corrupted packet size for filename");
        return;
    }

    // 2. Extract filename string
    std::string filename = rxValue.substr(sizeof(uint32_t), filenameLen);

    // 3. Extract remainder as JSON payload data
    std::string jsonData = rxValue.substr(sizeof(uint32_t) + filenameLen);

    // Save states
    ble.receivedFilename = filename;
    ble.receivedRoute = jsonData;

    ble.routeAvailable = true;    

    DBG_EXT(DBG_INFO, "[BLE] Filename: %s (%u bytes)", filename.c_str(), filenameLen);
    DBG_EXT(DBG_INFO, "[BLE] RX payload size: %u bytes", static_cast<unsigned>(jsonData.size()));
}

bool Ble::initDone()
{
    return bleInitialized;
}