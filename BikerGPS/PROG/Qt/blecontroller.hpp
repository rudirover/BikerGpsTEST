#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothDeviceInfo>
#include <QQueue>
#include <QList>
#include <QString>
#include <QByteArray>

struct BleFilePayload {
    QString filename;
    QByteArray data;
};

class BleController : public QObject {
    Q_OBJECT
public:
    explicit BleController(QObject *parent = nullptr);
    ~BleController() override;

    void connectAndSendFiles(const QString &targetName, const QList<BleFilePayload> &files);
    void loadConfig();

signals:
    void logMessage(const QString &message);
    void progressChanged(int value, const QString &statusText);
    void transmissionFinished(bool success);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onServiceDiscovered(const QBluetoothUuid &gattValue);

private:
    void cleanUpConnection();
    void sendNextChunk();

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_activeService = nullptr;

    QQueue<BleFilePayload> m_fileQueue;
    BleFilePayload m_currentFile;
    int m_currentFileOffset = 0;

    enum TransferState { Idle, SendingStart, SendingData, SendingEnd };
    TransferState m_transferState = Idle;

    bool m_deviceFound = false;
    QString m_targetDeviceName;

    int m_discoveryTimeoutMs = 7000;
    QString m_serviceUuid = "0000180f-0000-1000-8000-00805f9b34fb";
    QString m_characteristicUuid = "00002a19-0000-1000-8000-00805f9b34fb";
    bool m_writeWithResponse = true;
};