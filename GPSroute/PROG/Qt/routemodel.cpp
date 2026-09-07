#include "routemodel.h"
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
    if (diff > 180.0) {
        diff = 360.0 - diff;
    }
    return diff;
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
            out << "[Bluetooth]\n";
            out << "# Timeout in milliseconds for BLE discovery scanning\n";
            out << "DiscoveryTimeoutMs=7000\n";
            out << "# Target BLE peripheral device name\n";
            out << "DeviceName=BikerNetworkTool\n";
            out << "# Target GATT Service UUID\n";
            out << "ServiceUuid=0000180f-0000-1000-8000-00805f9b34fb\n";
            out << "# Target GATT Characteristic UUID\n";
            out << "CharacteristicUuid=00002a19-0000-1000-8000-00805f9b34fb\n";
            out << "# Enable GATT write with response\n";
            out << "WriteWithResponse=true\n\n";

            out << "[Overpass]\n";
            out << "# API endpoint URL for downloading bicycle network routes\n";
            out << "ApiUrl=https://overpass-api.de/api/interpreter\n";
            out << "# Server-side execution timeout in seconds\n";
            out << "QueryTimeoutSec=120\n";
            out << "# HTTP User-Agent identification string\n";
            out << "UserAgent=BikeNetworkTool/1.0 (Contact: rudih@project.local)\n";
            out << "# Minimum segment length threshold in meters\n";
            out << "MinSegmentDistanceMeters=50.0\n";
            out << "# Mean Earth radius in meters\n";
            out << "EarthRadiusMeters=6371000.0\n";
            out << "# Overpass query relation filter\n";
            out << "QueryFilter=relation[\"route\"=\"bicycle\"][\"network\"=\"rcn\"]\n";
            out << "# Distance measurement unit string\n";
            out << "DistanceUnit=km\n\n";

            out << "[Route]\n";
            out << "# Minimum turn angle in degrees (0-180) to trigger an auto-generated waypoint.\n";
            out << "TurnAngleThreshold=45.0\n";
            out << "# Minimum required distance in meters between consecutive auto-generated waypoints.\n";
            out << "MinDistBetweenWpts=15.0\n";
            out << "# Lookback and lookahead distance in meters used to calculate heading vectors around curves.\n";
            out << "SampleWindowMeters=5.0\n";
            out << "# Automatically write sent_route_log.json and sent_route_log.gpx during GPX import (true/false).\n";
            out << "AutoSaveLogs=true\n";
            out << "# Number of decimal places for latitude/longitude coordinate output.\n";
            out << "CoordinatePrecision=6\n";
            out << "# Mean Earth radius in meters used for Haversine distance calculations.\n";
            out << "EarthRadiusMeters=6371000.0\n";
            out << "# Filename for the exported JSON route log saved beside the application executable.\n";
            out << "JsonLogFilename=sent_route_log.json\n";
            out << "# Filename for the exported GPX route log saved beside the application executable.\n";
            out << "GpxLogFilename=sent_route_log.gpx\n";
            out << "# Metadata creator string written inside the GPX XML root element.\n";
            out << "GpxCreator=RouteModel\n";
            out << "# Starting integer used to name sequentially auto-generated waypoints.\n";
            out << "StartWaypointIndex=1\n";
            out << "# GPX schema version\n";
            out << "GpxVersion=1.1\n";
            out << "# GPX XML namespace\n";
            out << "GpxXmlns=http://www.topografix.com/GPX/1/1\n";
            out << "# Distance measurement unit string\n";
            out << "DistanceUnit=km\n";
            out << "# Fallback text displayed when segment distance is unknown\n";
            out << "MissingDistanceFallback=?? km\n\n";

            out << "[Web]\n";
            out << "# URL loaded in integrated web map browser\n";
            out << "MapViewerUrl=https://www.fietsknooppunt.be/nl-be/\n";
            out << "BunchiesUrl=https://www.bunchies.cc\n";
            out << "# Title for the web browser window\n";
            out << "WindowTitle=Fietsknooppunt Browser\n";
            out << "WindowWidth=944\n";
            out << "WindowHeight=880\n";
            out << "SnapOffsetPx=8\n\n";

            out << "[UI]\n";
            out << "WindowWidth=256\n";
            out << "WindowHeight=880\n";
            out << "ProgressResetDelayMs=5000\n";
            out << "StatusMessageDelayMs=3000\n";
            out << "DistanceSeparatorFormat=│  ▼  %1  ▼  │\n";
            out << "ListItemWidth=84\n";
            out << "ListItemHeight=84\n";
            out << "DistanceItemHeight=35\n";
            out << "BadgeSize=64\n";
            out << "BadgeBgColor=#23232a\n";
            out << "BadgeBorderColor=#10b981\n";
            out << "BadgeTextColor=#ffffff\n";
            out << "BadgeBorderWidth=4\n";
            out << "BadgeFontSize=24\n";
            out << "DistanceTextColor=#10b981\n";
            out << "ContainerMarginLeft=10\n";
            out << "ContainerMarginTop=4\n";
            out << "ContainerMarginRight=10\n";
            out << "ContainerMarginBottom=4\n";
            out << "GpxDownloadFilter=GPX Track (*.gpx);;All Files (*.*)\n";
            out << "GpxOpenFilter=GPX Files (*.gpx)\n";

            configFile.close();
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
    QString json = "[\n";
    for (int i = 0; i < m_currentPathJson.size(); ++i) {
        QJsonObject wptObj = m_currentPathJson[i].toObject();
        json += "  {\n";
        json += QString("    \"wpt\": \"%1\",\n").arg(wptObj["wpt"].toString());
        json += QString("    \"dist\": %1,\n").arg(qRound(wptObj["dist"].toDouble()));
        json += QString("    \"lat\": %1,\n").arg(QString::number(wptObj["lat"].toDouble(), 'f', m_coordinatePrecision));

        if (wptObj.contains("tpt")) {
            json += QString("    \"lon\": %1,\n").arg(QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
            json += "    \"tpt\": [\n";
            QJsonArray tptArray = wptObj["tpt"].toArray();
            for (int j = 0; j < tptArray.size(); ++j) {
                QJsonObject tptObj = tptArray[j].toObject();
                json += "      {\n";
                json += QString("        \"lat\": %1,\n").arg(QString::number(tptObj["lat"].toDouble(), 'f', m_coordinatePrecision));
                json += QString("        \"lon\": %1,\n").arg(QString::number(tptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
                json += QString("        \"dist\": %1\n").arg(qRound(tptObj["dist"].toDouble()));
                json += "      }";
                if (j < tptArray.size() - 1) json += ",";
                json += "\n";
            }
            json += "    ]\n";
        } else {
            json += QString("    \"lon\": %1\n").arg(QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
        }

        json += "  }";
        if (i < m_currentPathJson.size() - 1) json += ",";
        json += "\n";
    }
    json += "]\n";
    return json;
}

QStringList RouteModel::nodeNumbers() const {
    QStringList nodes;
    for (const QJsonValue &val : m_currentPathJson) {
        nodes.append(val.toObject()["wpt"].toString());
    }
    return nodes;
}

QStringList RouteModel::segmentDistances() const {
    QStringList distances;
    for (int i = 0; i < m_currentPathJson.size() - 1; ++i) {
        QJsonObject obj = m_currentPathJson[i].toObject();
        if (obj.contains("dist")) {
            double distMeters = obj["dist"].toDouble();
            double distKm = distMeters / 1000.0;
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

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return m_earthRadiusMeters * c;
}

bool RouteModel::importFromGpx(const QString &fileName) {
    loadConfig();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QXmlStreamReader xml(&file);
    QStringList nodes;
    QList<QPair<double, double>> waypoints;
    QList<QPair<double, double>> trackPoints;

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
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
    }

    if (waypoints.isEmpty()) {
        if (trackPoints.isEmpty()) return false;

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

            double angleTurn = calculateAngleDiff(bIn, bOut);

            if (angleTurn >= m_turnAngleThreshold) {
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
                segmentDist += getHaversineDistance(
                    trackPoints[k].first, trackPoints[k].second,
                    trackPoints[k + 1].first, trackPoints[k + 1].second
                    );
            }
            wptObj["dist"] = std::round(segmentDist);

            QJsonArray segArray;
            for (int k = trkIdx; k <= nextIdx && k < trackPoints.size(); ++k) {
                QJsonObject ptObj;
                ptObj["lat"] = trackPoints[k].first;
                ptObj["lon"] = trackPoints[k].second;

                if (k < nextIdx && k < trackPoints.size() - 1) {
                    double ptDist = getHaversineDistance(
                        trackPoints[k].first, trackPoints[k].second,
                        trackPoints[k + 1].first, trackPoints[k + 1].second
                        );
                    ptObj["dist"] = std::round(ptDist);
                } else {
                    ptObj["dist"] = 0.0;
                }

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
        saveRouteLogs();
    }

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

    for (const QJsonValue &val : m_currentPathJson) {
        QJsonObject wptObj = val.toObject();
        xml.writeStartElement("wpt");
        xml.writeAttribute("lat", QString::number(wptObj["lat"].toDouble(), 'f', m_coordinatePrecision));
        xml.writeAttribute("lon", QString::number(wptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
        xml.writeTextElement("name", wptObj["wpt"].toString());
        xml.writeEndElement();
    }

    xml.writeStartElement("trk");
    xml.writeStartElement("trkseg");

    for (const QJsonValue &val : m_currentPathJson) {
        QJsonObject wptObj = val.toObject();
        if (wptObj.contains("tpt")) {
            QJsonArray tptArray = wptObj["tpt"].toArray();
            for (const QJsonValue &tptVal : tptArray) {
                QJsonObject tptObj = tptVal.toObject();
                xml.writeStartElement("trkpt");
                xml.writeAttribute("lat", QString::number(tptObj["lat"].toDouble(), 'f', m_coordinatePrecision));
                xml.writeAttribute("lon", QString::number(tptObj["lon"].toDouble(), 'f', m_coordinatePrecision));
                xml.writeEndElement();
            }
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
        jsonFile.close();
    }

    return exportToGpx(gpxPath);
}