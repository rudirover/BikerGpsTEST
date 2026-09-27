#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothDeviceInfo>

class BleController : public QObject {
    Q_OBJECT
public:
    explicit BleController(QObject *parent = nullptr);
    ~BleController();

    void connectAndSend(const QString &targetName, const QString &filename, const QByteArray &jsonData);
    void loadConfig();

signals:
    void logMessage(const QString &message);
    void progressChanged(int value, const QString &statusText);
    void transmissionFinished(bool success);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onServiceDiscovered(const QBluetoothUuid &gattValue);
    void onServiceDiscoveryFinished();

private:
    void cleanUpConnection();

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller;
    QLowEnergyService *m_activeService;

    QString m_targetDeviceName;
    QString m_pendingFilename;
    QByteArray m_pendingData;
    bool m_deviceFound;

    int m_discoveryTimeoutMs = 7000;
    QString m_serviceUuid = "0000180f-0000-1000-8000-00805f9b34fb";
    QString m_characteristicUuid = "00002a19-0000-1000-8000-00805f9b34fb";
    bool m_writeWithResponse = true;
};