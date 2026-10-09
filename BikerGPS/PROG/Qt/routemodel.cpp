#include "routemodel.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>
#include <cmath>
#include <limits>

static double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    double dLon = qDegreesToRadians(lon2 - lon1);
    double rLat1 = qDegreesToRadians(lat1);
    double rLat2 = qDegreesToRadians(lat2);

    double y = std::sin(dLon) * std::cos(rLat2);
    double x = std::cos(rLat1) * std::sin(rLat2) -
               std::sin(rLat1) * std::cos(rLat2) * std::cos(dLon);

    double brng = qRadiansToDegrees(std::atan2(y, x));
    return std::fmod(brng + 360.0, 360.0);
}

static double calculateAngleDiff(double b1, double b2) {
    double diff = std::abs(b2 - b1);
    return (diff > 180.0) ? (360.0 - diff) : diff;
}

RouteModel::RouteModel(QObject *parent) : QObject(parent) {
    loadConfig();
}

void RouteModel::loadConfig() {
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");

    if (!QFile::exists(configPath)) {
        QFile configFile(configPath);
        if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&configFile);
            out << "[Bluetooth]\n"
                << "DiscoveryTimeoutMs=7000\n"
                << "DeviceName=BikerNetworkTool\n"
                << "ServiceUuid=0000180f-0000-1000-8000-00805f9b34fb\n"
                << "CharacteristicUuid=00002a19-0000-1000-8000-00805f9b34fb\n"
                << "WriteWithResponse=true\n\n"
                << "[Overpass]\n"
                << "ApiUrl=https://overpass-api.de/api/interpreter\n"
                << "QueryTimeoutSec=120\n"
                << "UserAgent=BikeNetworkTool/1.0 (Contact: rudih@project.local)\n"
                << "MinSegmentDistanceMeters=50.0\n"
                << "EarthRadiusMeters=6371000.0\n"
                << "QueryFilter=relation[\"route\"=\"bicycle\"][\"network\"=\"rcn\"]\n"
                << "DistanceUnit=km\n\n"
                << "[Route]\n"
                << "TurnAngleThreshold=45.0\n"
                << "MinDistBetweenWpts=15.0\n"
                << "SampleWindowMeters=5.0\n"
                << "AutoSaveLogs=true\n"
                << "CoordinatePrecision=6\n"
                << "EarthRadiusMeters=6371000.0\n"
                << "JsonLogFilename=sent_route_log.json\n"
                << "GpxLogFilename=sent_route_log.gpx\n"
                << "MapLogFilename=sent_route_log.bma2\n"
                << "GpxCreator=RouteModel\n"
                << "StartWaypointIndex=1\n"
                << "GpxVersion=1.1\n"
                << "GpxXmlns=http://www.topografix.com/GPX/1/1\n"
                << "DistanceUnit=km\n"
                << "MissingDistanceFallback=?? km\n\n"
                << "[Web]\n"
                << "MapViewerUrl=https://www.fietsknooppunt.be/nl-be/\n"
                << "BunchiesUrl=https://www.bunchies.cc\n"
                << "WindowTitle=Fietsknooppunt Browser\n"
                << "WindowWidth=944\n"
                << "WindowHeight=880\n"
                << "SnapOffsetPx=8\n\n"
                << "[UI]\n"
                << "WindowWidth=256\n"
                << "WindowHeight=880\n";
        }
    }

    QSettings settings(configPath, QSettings::IniFormat);
    m_turnAngleThreshold = settings.value("Route/TurnAngleThreshold", 45.0).toDouble();
    m_minDistBetweenWpts = settings.value("Route/MinDistBetweenWpts", 15.0).toDouble();
    m_sampleWindowMeters = settings.value("Route/SampleWindowMeters", 5.0).toDouble();
    m_autoSaveLogs = settings.value("Route/AutoSaveLogs", true).toBool();
    m_coordinatePrecision = settings.value("Route/CoordinatePrecision", 6).toInt();
    m_earthRadiusMeters = settings.value("Route/EarthRadiusMeters", 6371000.0).toDouble();
    m_jsonLogFilename = settings.value("Route/JsonLogFilename", "sent_route_log.json").toString();
    m_gpxLogFilename = settings.value("Route/GpxLogFilename", "sent_route_log.gpx").toString();
    m_mapLogFilename = settings.value("Route/MapLogFilename", "sent_route_log.bma2").toString();
    m_gpxCreator = settings.value("Route/GpxCreator", "RouteModel").toString();
    m_startWaypointIndex = settings.value("Route/StartWaypointIndex", 1).toInt();
    m_gpxVersion = settings.value("Route/GpxVersion", "1.1").toString();
    m_gpxXmlns = settings.value("Route/GpxXmlns", "http://www.topografix.com/GPX/1/1").toString();
    m_distanceUnit = settings.value("Route/DistanceUnit", "km").toString();
    m_missingDistanceFallback = settings.value("Route/MissingDistanceFallback", "?? km").toString();
}

bool RouteModel::isEmpty() const {
    return m_currentPathJson.isEmpty();
}

QJsonArray RouteModel::currentPathJson() const {
    return m_currentPathJson;
}

QString RouteModel::toOrderedJsonString() const {
    QString jsonOutput;
    QTextStream json(&jsonOutput);
    json << "[\n";
    for (int i = 0; i < m_currentPathJson.size(); ++i) {
        QJsonObject wptObj = m_currentPathJson[i].toObject();
        json << "  {\n"
             << "    \"wpt\": \"" << wptObj["wpt"].toString() << "\",\n"
             << "    \"dist\": " << qRound(wptObj["dist"].toDouble()) << ",\n"
             << "    \"lat\": " << QString::number(wptObj["lat"].toDouble(), 'f', m_coordinatePrecision) << ",\n";

        if (wptObj.contains("tpt")) {
            json << "    \"lon\": " << QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision) << ",\n"
                 << "    \"tpt\": [\n";
            QJsonArray tptArray = wptObj["tpt"].toArray();
            for (int j = 0; j < tptArray.size(); ++j) {
                QJsonObject tptObj = tptArray[j].toObject();
                json << "      {\n"
                     << "        \"lat\": " << QString::number(tptObj["lat"].toDouble(), 'f', m_coordinatePrecision) << ",\n"
                     << "        \"lon\": " << QString::number(tptObj["lon"].toDouble(), 'f', m_coordinatePrecision) << ",\n"
                     << "        \"dist\": " << qRound(tptObj["dist"].toDouble()) << "\n"
                     << "      }";
                if (j < tptArray.size() - 1) json << ",";
                json << "\n";
            }
            json << "    ]\n";
        } else {
            json << "    \"lon\": " << QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision) << "\n";
        }

        json << "  }";
        if (i < m_currentPathJson.size() - 1) json << ",";
        json << "\n";
    }
    json << "]\n";
    return jsonOutput;
}

QStringList RouteModel::nodeNumbers() const {
    QStringList nodes;
    for (const auto &val : m_currentPathJson) {
        nodes.append(val.toObject()["wpt"].toString());
    }
    return nodes;
}

QStringList RouteModel::segmentDistances() const {
    QStringList distances;
    for (int i = 0; i < m_currentPathJson.size() - 1; ++i) {
        QJsonObject obj = m_currentPathJson[i].toObject();
        if (obj.contains("dist")) {
            double distKm = obj["dist"].toDouble() / 1000.0;
            distances.append(QString::number(distKm, 'f', 1) + " " + m_distanceUnit);
        } else {
            distances.append(m_missingDistanceFallback);
        }
    }
    return distances;
}

double RouteModel::getHaversineDistance(double lat1, double lon1, double lat2, double lon2) const {
    double dLat = qDegreesToRadians(lat2 - lat1);
    double dLon = qDegreesToRadians(lon2 - lon1);

    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(qDegreesToRadians(lat1)) * std::cos(qDegreesToRadians(lat2)) *
                   std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

    return m_earthRadiusMeters * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

bool RouteModel::importFromGpx(const QString &fileName) {
    loadConfig();

    emit statusMessage("Opening GPX File...");
    emit progressUpdated(5);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to open GPX file for reading.");
        return false;
    }

    QXmlStreamReader xml(&file);
    QStringList nodes;
    QList<QPair<double, double>> waypoints;
    QList<QPair<double, double>> trackPoints;

    emit statusMessage("Parsing XML Elements...");
    emit progressUpdated(15);

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNext() != QXmlStreamReader::StartElement) continue;

        QString name = xml.name().toString();
        if (name == "wpt") {
            waypoints.append({xml.attributes().value("lat").toDouble(),
                              xml.attributes().value("lon").toDouble()});
            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString() == "wpt")) {
                if (xml.readNext() == QXmlStreamReader::StartElement && xml.name().toString() == "name") {
                    nodes.append(xml.readElementText().trimmed());
                }
            }
        } else if (name == "trkpt") {
            trackPoints.append({xml.attributes().value("lat").toDouble(),
                                xml.attributes().value("lon").toDouble()});
        }
    }

    if (waypoints.isEmpty()) {
        if (trackPoints.isEmpty()) {
            emit errorOccurred("GPX file contains no valid waypoints or track points.");
            return false;
        }

        emit statusMessage("Analyzing Track...");
        emit progressUpdated(30);

        int wptCounter = m_startWaypointIndex;
        waypoints.append(trackPoints.first());
        nodes.append(QString::number(wptCounter++));

        int lastWptTrkIdx = 0;
        for (int i = 1; i < trackPoints.size() - 1; ++i) {
            int prevIdx = i - 1;
            while (prevIdx > 0 && getHaversineDistance(trackPoints[prevIdx].first, trackPoints[prevIdx].second,
                                                       trackPoints[i].first, trackPoints[i].second) < m_sampleWindowMeters) {
                prevIdx--;
            }

            int nextIdx = i + 1;
            while (nextIdx < trackPoints.size() - 1 && getHaversineDistance(trackPoints[i].first, trackPoints[i].second,
                                                                            trackPoints[nextIdx].first, trackPoints[nextIdx].second) < m_sampleWindowMeters) {
                nextIdx++;
            }

            double bIn = calculateBearing(trackPoints[prevIdx].first, trackPoints[prevIdx].second,
                                          trackPoints[i].first, trackPoints[i].second);
            double bOut = calculateBearing(trackPoints[i].first, trackPoints[i].second,
                                           trackPoints[nextIdx].first, trackPoints[nextIdx].second);

            if (calculateAngleDiff(bIn, bOut) >= m_turnAngleThreshold) {
                double distFromLastWpt = getHaversineDistance(trackPoints[lastWptTrkIdx].first, trackPoints[lastWptTrkIdx].second,
                                                              trackPoints[i].first, trackPoints[i].second);
                if (distFromLastWpt >= m_minDistBetweenWpts) {
                    waypoints.append(trackPoints[i]);
                    nodes.append(QString::number(wptCounter++));
                    lastWptTrkIdx = i;
                }
            }
        }

        if (waypoints.last() != trackPoints.last()) {
            waypoints.append(trackPoints.last());
            nodes.append(QString::number(wptCounter++));
        }
    }

    emit statusMessage("Computing Segments...");
    emit progressUpdated(45);

    m_currentPathJson = QJsonArray();
    int trkIdx = 0;

    for (int i = 0; i < waypoints.size(); ++i) {
        QJsonObject wptObj;
        wptObj["wpt"] = (i < nodes.size()) ? nodes[i] : QString::number(i + m_startWaypointIndex);
        wptObj["lat"] = waypoints[i].first;
        wptObj["lon"] = waypoints[i].second;

        if (i < waypoints.size() - 1) {
            auto nextWpt = waypoints[i + 1];
            int nextIdx = trkIdx;
            double minDistance = std::numeric_limits<double>::max();

            for (int k = trkIdx; k < trackPoints.size(); ++k) {
                double dist = getHaversineDistance(trackPoints[k].first, trackPoints[k].second,
                                                   nextWpt.first, nextWpt.second);
                if (dist < minDistance) {
                    minDistance = dist;
                    nextIdx = k;
                }
            }

            double segmentDist = 0.0;
            for (int k = trkIdx; k < nextIdx && k < trackPoints.size() - 1; ++k) {
                segmentDist += getHaversineDistance(trackPoints[k].first, trackPoints[k].second,
                                                    trackPoints[k + 1].first, trackPoints[k + 1].second);
            }
            wptObj["dist"] = std::round(segmentDist);

            QJsonArray segArray;
            for (int k = trkIdx; k <= nextIdx && k < trackPoints.size(); ++k) {
                QJsonObject ptObj;
                ptObj["lat"] = trackPoints[k].first;
                ptObj["lon"] = trackPoints[k].second;
                double ptDist = (k < nextIdx && k < trackPoints.size() - 1) ?
                                    getHaversineDistance(trackPoints[k].first, trackPoints[k].second,
                                                         trackPoints[k + 1].first, trackPoints[k + 1].second) : 0.0;
                ptObj["dist"] = std::round(ptDist);
                segArray.append(ptObj);
            }

            wptObj["tpt"] = segArray;
            trkIdx = nextIdx;
        } else {
            wptObj["dist"] = 0.0;
        }

        m_currentPathJson.append(wptObj);
    }

    if (m_autoSaveLogs) {
        emit statusMessage("Saving Logs...");
        emit progressUpdated(55);
        saveRouteLogs();
    }

    emit statusMessage("Compiling Map..");
    emit progressUpdated(65);
    compileMapFromImportedGpx();

    return true;
}

bool RouteModel::exportToGpx(const QString &fileName) const {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("gpx");
    xml.writeAttribute("version", m_gpxVersion);
    xml.writeAttribute("creator", m_gpxCreator);
    xml.writeAttribute("xmlns", m_gpxXmlns);

    for (const auto &val : m_currentPathJson) {
        QJsonObject wptObj = val.toObject();
        xml.writeStartElement("wpt");
        xml.writeAttribute("lat", QString::number(wptObj["lat"].toDouble(), 'f', m_coordinatePrecision));
        xml.writeAttribute("lon", QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
        xml.writeTextElement("name", wptObj["wpt"].toString());
        xml.writeEndElement();
    }

    xml.writeStartElement("trk");
    xml.writeStartElement("trkseg");

    for (const auto &val : m_currentPathJson) {
        QJsonObject wptObj = val.toObject();
        if (!wptObj.contains("tpt")) continue;

        QJsonArray tptArray = wptObj["tpt"].toArray();
        for (const auto &tptVal : tptArray) {
            QJsonObject tptObj = tptVal.toObject();
            xml.writeStartElement("trkpt");
            xml.writeAttribute("lat", QString::number(tptObj["lat"].toDouble(), 'f', m_coordinatePrecision));
            xml.writeAttribute("lon", QString::number(tptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
            xml.writeEndElement();
        }
    }

    xml.writeEndElement(); // trkseg
    xml.writeEndElement(); // trk
    xml.writeEndElement(); // gpx
    xml.writeEndDocument();

    return true;
}

bool RouteModel::saveRouteLogs() const {
    if (m_currentPathJson.isEmpty()) return false;

    QString appDir = QCoreApplication::applicationDirPath();
    QString jsonPath = QDir(appDir).filePath(m_jsonLogFilename);
    QString gpxPath = QDir(appDir).filePath(m_gpxLogFilename);

    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        jsonFile.write(toOrderedJsonString().toUtf8());
    }

    return exportToGpx(gpxPath);
}

bool RouteModel::compileMapFromImportedGpx(bool buildings, bool green, bool water, bool railways) {
    if (m_currentPathJson.isEmpty()) return false;

    double minLat = 90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;

    for (const auto &val : m_currentPathJson) {
        QJsonObject wptObj = val.toObject();
        minLat = qMin(minLat, wptObj["lat"].toDouble());
        maxLat = qMax(maxLat, wptObj["lat"].toDouble());
        minLon = qMin(minLon, wptObj["lon"].toDouble());
        maxLon = qMax(maxLon, wptObj["lon"].toDouble());

        if (!wptObj.contains("tpt")) continue;

        QJsonArray tptArray = wptObj["tpt"].toArray();
        for (const auto &tptVal : tptArray) {
            QJsonObject tptObj = tptVal.toObject();
            minLat = qMin(minLat, tptObj["lat"].toDouble());
            maxLat = qMax(maxLat, tptObj["lat"].toDouble());
            minLon = qMin(minLon, tptObj["lon"].toDouble());
            maxLon = qMax(maxLon, tptObj["lon"].toDouble());
        }
    }

    double padding = 0.005;
    minLat -= padding; maxLat += padding;
    minLon -= padding; maxLon += padding;

    QJsonObject firstWpt = m_currentPathJson.first().toObject();
    double originLat = firstWpt["lat"].toDouble();
    double originLon = firstWpt["lon"].toDouble();

    QString mapPath = QDir(QCoreApplication::applicationDirPath()).filePath(m_mapLogFilename);
    auto *preprocessor = new MapPreprocessor(this);

    connect(preprocessor, &MapPreprocessor::statusMessage, this, &RouteModel::statusMessage);
    connect(preprocessor, &MapPreprocessor::progressUpdated, this, [this](int percent) {
        emit progressUpdated(65 + (percent * 35) / 100);
    });
    connect(preprocessor, &MapPreprocessor::errorOccurred, this, &RouteModel::errorOccurred);
    connect(preprocessor, &MapPreprocessor::downloadFinished, this, [this, preprocessor](bool success) {
        preprocessor->deleteLater();
        emit mapCompilationFinished(success);
    });

    preprocessor->downloadAndProcessCustomBounds(minLat, minLon, maxLat, maxLon, originLat, originLon, mapPath, buildings, green, water, railways);

    return true;
}