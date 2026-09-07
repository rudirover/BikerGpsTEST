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
            DBG_EXT(DBG_INFO, "Device initialized");

            pServer = BLEDevice::createServer();
            pServer->setCallbacks(new MyServerCallbacks());
            DBG_EXT(DBG_INFO, "Server created");

            pService = pServer->createService(BLE_SERVICE_UUID);
            DBG_EXT(DBG_INFO, "Service created");

            pCharacteristic = pService->createCharacteristic(
                BLE_CHARACTERISTIC_UUID,
                BLECharacteristic::PROPERTY_WRITE);

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

    // First time BLE initialization - create the entire BLE stack
    if (!bleInitialized)
    {
        DBG_EXT(DBG_WARNING, "Stack not initialized");
    }
    else
    {
        // BLE already initialized, just restart advertising
        DBG_EXT(DBG_INFO, "Reusing stack, restarting advertising");
        if (pAdvertising)
        {
            // Ensure clean state before starting
            if (deviceConnected)
            {
                DBG_EXT(DBG_INFO, "Waiting for stale connection to timeout");
                // Let connection naturally timeout rather than forcing disconnect
                delay(50);
            }
        }
    }

    // Start advertising
    if (pAdvertising)
    {
        try
        {
            pAdvertising->start();
            ble.enabled = true;
            String address = String(BLEDevice::getAddress().toString().c_str());
            DBG_EXT(DBG_INFO, "Advertising started");
            DBG_EXT(DBG_INFO, "Address: %s", address);
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
            Serial.println("[BLE] Advertising stopped");
        }
        catch (...)
        {
            Serial.println("[BLE] ERROR: Failed to stop advertising");
        }
    }

    // Disconnect any connected devices
    if (deviceConnected && pServer)
    {
        try
        {
            Serial.println("[BLE] Disconnecting device...");
            // Force disconnect all client peer connections
            pServer->disconnect(pServer->getConnId());
        }
        catch (...)
        {
            Serial.println("[BLE] Error during disconnect handling");
        }
    }
    ble.enabled = false;
    Serial.println("[BLE] BLE disabled (stack kept for reuse)");
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

    ble.receivedRoute = pChar->getValue();
    ble.routeAvailable = true; // here we should first check if contents is really a route

    Serial.printf("[BLE] RX payload size: %u bytes\n", static_cast<unsigned>(ble.receivedRoute.size()));
    /*
    Serial.print("[BLE] RX: ");
    Serial.println(ble.receivedRoute.c_str());
    */
}

bool Ble::initDone()
{
    return bleInitialized;
}