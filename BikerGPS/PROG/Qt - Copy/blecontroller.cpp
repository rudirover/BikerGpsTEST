#include "blecontroller.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

BleController::BleController(QObject *parent)
    : QObject(parent), m_controller(nullptr), m_activeService(nullptr), m_deviceFound(false) {

    loadConfig();

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(m_discoveryTimeoutMs);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BleController::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BleController::onScanFinished);
}

BleController::~BleController() {
    cleanUpConnection();
}

void BleController::loadConfig() {
    QString configPath = QDir(qApp->applicationDirPath()).filePath("config.ini");
    QSettings settings(configPath, QSettings::IniFormat);

    m_discoveryTimeoutMs = settings.value("Bluetooth/DiscoveryTimeoutMs", 7000).toInt();
    m_serviceUuid = settings.value("Bluetooth/ServiceUuid", "0000180f-0000-1000-8000-00805f9b34fb").toString();
    m_characteristicUuid = settings.value("Bluetooth/CharacteristicUuid", "00002a19-0000-1000-8000-00805f9b34fb").toString();
    m_writeWithResponse = settings.value("Bluetooth/WriteWithResponse", true).toBool();
}

void BleController::cleanUpConnection() {
    if (m_activeService) {
        m_activeService->deleteLater();
        m_activeService = nullptr;
    }
    if (m_controller) {
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
}

void BleController::connectAndSend(const QString &targetName, const QString &filename, const QByteArray &jsonData) {
    loadConfig();
    cleanUpConnection();
    m_targetDeviceName = targetName;
    m_pendingFilename = filename;
    m_pendingData = jsonData;
    m_deviceFound = false;

    emit progressChanged(10, "Scanning for device...");
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BleController::onDeviceDiscovered(const QBluetoothDeviceInfo &device) {
    if (m_deviceFound) return;

    qDebug() << "Device found: " << device.name();

    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        if (device.name() == m_targetDeviceName) {
            m_deviceFound = true;
            m_discoveryAgent->stop();

            emit progressChanged(40, "Device found. Connecting...");

            m_controller = QLowEnergyController::createCentral(device, this);

            connect(m_controller, &QLowEnergyController::mtuChanged, this, [this](int mtu) {
                emit logMessage("Active MTU updated to: " + QString::number(mtu));
            });

            connect(m_controller, &QLowEnergyController::connected, this, [this]() {
                emit progressChanged(60, "Connected. Discovering services...");
                m_controller->discoverServices();
            });

            connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
                emit logMessage("Disconnected from peripheral device.");
            });

            connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &BleController::onServiceDiscovered);
            connect(m_controller, &QLowEnergyController::discoveryFinished, this, &BleController::onServiceDiscoveryFinished);

            connect(m_controller, &QLowEnergyController::errorOccurred, this, [this](QLowEnergyController::Error error){
                emit logMessage("Controller Error: " + QString::number(error));
                emit transmissionFinished(false);
            });

            m_controller->connectToDevice();
        }
    }
}

void BleController::onScanFinished() {
    if (!m_deviceFound) {
        emit progressChanged(0, "Device not found.");
        emit transmissionFinished(false);
    }
}

void BleController::onServiceDiscovered(const QBluetoothUuid &gattValue) {
    if (gattValue == QBluetoothUuid(m_serviceUuid)) {
        m_activeService = m_controller->createServiceObject(gattValue, this);

        if (m_activeService) {
            connect(m_activeService, &QLowEnergyService::characteristicWritten,
                    this, [this](const QLowEnergyCharacteristic &ch, const QByteArray &value) {
                        emit progressChanged(100, "Data sent successfully!");
                        emit transmissionFinished(true);
                        cleanUpConnection();
                    });

            connect(m_activeService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state) {
                if (state == QLowEnergyService::RemoteServiceDiscovered) {
                    emit progressChanged(80, "Service ready. Transmitting...");

                    QLowEnergyCharacteristic characteristic = m_activeService->characteristic(QBluetoothUuid(m_characteristicUuid));

                    if (!characteristic.isValid()) {
                        emit progressChanged(0, "Characteristic error.");
                        emit transmissionFinished(false);
                        return;
                    }

                    // Build custom protocol packet safely
                    QByteArray payloadPacket;
                    QByteArray filenameBytes = m_pendingFilename.toUtf8();
                    quint32 filenameLen = static_cast<quint32>(filenameBytes.size());

                    payloadPacket.append(reinterpret_cast<const char*>(&filenameLen), sizeof(filenameLen));
                    payloadPacket.append(filenameBytes);
                    payloadPacket.append(m_pendingData);

                    int currentMtu = m_controller->mtu();
                    emit logMessage(QString("Payload size: %1 bytes | Current MTU limit: %2").arg(payloadPacket.size()).arg(currentMtu));

                    QLowEnergyService::WriteMode mode = m_writeWithResponse ? QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse;

                    m_activeService->writeCharacteristic(characteristic, payloadPacket, mode);
                }
            });

            m_activeService->discoverDetails();
        }
    }
}

void BleController::onServiceDiscoveryFinished() {
}