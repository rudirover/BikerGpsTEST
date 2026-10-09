#include "blecontroller.hpp"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDataStream>

BleController::BleController(QObject *parent)
    : QObject(parent) {
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
    m_transferState = Idle;
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

void BleController::connectAndSendFiles(const QString &targetName, const QList<BleFilePayload> &files) {
    loadConfig();
    cleanUpConnection();

    m_targetDeviceName = targetName;
    m_fileQueue.clear();
    for (const auto &file : files) {
        m_fileQueue.enqueue(file);
    }
    m_deviceFound = false;

    if (m_fileQueue.isEmpty()) {
        emit transmissionFinished(false);
        return;
    }

    emit progressChanged(10, "Scanning...");
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BleController::onDeviceDiscovered(const QBluetoothDeviceInfo &device) {
    if (m_deviceFound) return;

    if ((device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) &&
        (device.name() == m_targetDeviceName)) {

        m_deviceFound = true;
        m_discoveryAgent->stop();
        emit progressChanged(40, "Connecting...");

        m_controller = QLowEnergyController::createCentral(device, this);
        connect(m_controller, &QLowEnergyController::connected, this, [this]() {
            emit progressChanged(60, "Discovering...");
            m_controller->discoverServices();
        });
        connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
            emit logMessage("Disconnected from peripheral device.");
        });
        connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &BleController::onServiceDiscovered);
        connect(m_controller, &QLowEnergyController::errorOccurred, this, [this](QLowEnergyController::Error error){
            emit logMessage("Controller Error: " + QString::number(error));
            emit transmissionFinished(false);
        });

        m_controller->connectToDevice();
    }
}

void BleController::onScanFinished() {
    if (!m_deviceFound) {
        emit progressChanged(0, "Not Found.");
        emit transmissionFinished(false);
    }
}

void BleController::sendNextChunk() {
    if (!m_activeService) return;

    QLowEnergyCharacteristic characteristic = m_activeService->characteristic(QBluetoothUuid(m_characteristicUuid));
    if (!characteristic.isValid()) {
        emit progressChanged(0, "Char Error.");
        emit transmissionFinished(false);
        return;
    }

    QByteArray packet;
    QLowEnergyService::WriteMode mode = m_writeWithResponse ?
                                            QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse;

    if (m_transferState == SendingStart) {
        packet.append(static_cast<char>(0x01)); // START_FILE
        quint32 fileSize = m_currentFile.data.size();
        packet.append(reinterpret_cast<const char*>(&fileSize), sizeof(quint32));
        QByteArray fnameBytes = m_currentFile.filename.toUtf8();
        quint32 fnameLen = fnameBytes.size();
        packet.append(reinterpret_cast<const char*>(&fnameLen), sizeof(quint32));
        packet.append(fnameBytes);

        emit logMessage(QString("Starting file: %1").arg(m_currentFile.filename));
        m_transferState = SendingData;
        m_currentFileOffset = 0;
        m_activeService->writeCharacteristic(characteristic, packet, mode);
    }
    else if (m_transferState == SendingData) {
        int chunkSize = 500; // Safe chunk payload size below MTU limits
        int remaining = m_currentFile.data.size() - m_currentFileOffset;

        if (remaining > 0) {
            int currentSize = qMin(chunkSize, remaining);
            packet.append(static_cast<char>(0x02)); // DATA_CHUNK
            packet.append(m_currentFile.data.mid(m_currentFileOffset, currentSize));
            m_currentFileOffset += currentSize;

            int progress = 70 + static_cast<int>(30.0 * m_currentFileOffset / m_currentFile.data.size());
            emit progressChanged(progress, "Transmitting...");

            m_activeService->writeCharacteristic(characteristic, packet, mode);
        } else {
            m_transferState = SendingEnd;
            sendNextChunk(); // Automatically trigger end packet
        }
    }
    else if (m_transferState == SendingEnd) {
        packet.append(static_cast<char>(0x03)); // END_FILE
        emit logMessage(QString("Finished file: %1").arg(m_currentFile.filename));

        if (!m_fileQueue.isEmpty()) {
            m_currentFile = m_fileQueue.dequeue();
            m_transferState = SendingStart;
        } else {
            m_transferState = Idle;
            emit progressChanged(100, "Sent!");
            emit transmissionFinished(true);
            cleanUpConnection();
            return;
        }
        m_activeService->writeCharacteristic(characteristic, packet, mode);
    }
}

void BleController::onServiceDiscovered(const QBluetoothUuid &gattValue) {
    if (gattValue != QBluetoothUuid(m_serviceUuid)) return;

    m_activeService = m_controller->createServiceObject(gattValue, this);
    if (!m_activeService) return;

    connect(m_activeService, &QLowEnergyService::characteristicWritten,
            this, [this]() {
                sendNextChunk(); // Chain next chunk upon confirmation write response
            });

    connect(m_activeService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state) {
        if (state != QLowEnergyService::RemoteServiceDiscovered) return;

        emit progressChanged(70, "Transmitting...");

        if (m_fileQueue.isEmpty()) {
            emit transmissionFinished(false);
            cleanUpConnection();
            return;
        }

        m_currentFile = m_fileQueue.dequeue();
        m_transferState = SendingStart;
        sendNextChunk();
    });

    m_activeService->discoverDetails();
}