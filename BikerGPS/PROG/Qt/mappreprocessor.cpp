#include "mappreprocessor.hpp"
#include <QFile>
#include <QUrl>
#include <QNetworkRequest>
#include <QtMath>
#include <QNetworkProxy>
#include <QDebug>
#include <QUrlQuery>
#include <QDataStream>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <cstring>
#include <functional>

MapPreprocessor::MapPreprocessor(QObject *parent)
    : QObject(parent)
{
    loadConfig();

    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &MapPreprocessor::onDownloadReply);
}

void MapPreprocessor::loadConfig() {
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    QSettings settings(configPath, QSettings::IniFormat);

    m_apiUrls.clear();
    for (int i = 1; i <= 10; ++i) {
        QString key = QString("Overpass/Endpoint%1").arg(i);
        QString url = settings.value(key, "").toString().trimmed();
        if (url.endsWith('"')) {
            url.chop(1); // Clean up trailing quotes if accidentally present in INI values
        }
        if (!url.isEmpty()) {
            m_apiUrls.append(url);
        }
    }

    if (m_apiUrls.isEmpty()) {
        m_apiUrls.append("https://overpass-api.de/api/interpreter");
    }
}

void MapPreprocessor::calculateBoundingBox(double lat, double lon, double &minLat, double &minLon, double &maxLat, double &maxLon) {
    double deltaLat = 2500.0 / 111000.0;
    double deltaLon = 2500.0 / (111000.0 * qCos(qDegreesToRadians(lat)));

    minLat = lat - deltaLat;
    maxLat = lat + deltaLat;
    minLon = lon - deltaLon;
    maxLon = lon + deltaLon;
}

void MapPreprocessor::downloadAndProcess(double originLat, double originLon, const QString &outputPath,
                                         bool includeBuildings, bool includeGreenZones, bool includeWater, bool includeRailways) {
    double minLat, minLon, maxLat, maxLon;
    calculateBoundingBox(originLat, originLon, minLat, minLon, maxLat, maxLon);
    downloadAndProcessCustomBounds(minLat, minLon, maxLat, maxLon, originLat, originLon, outputPath, includeBuildings, includeGreenZones, includeWater, includeRailways);
}

void MapPreprocessor::downloadAndProcessCustomBounds(double minLat, double minLon, double maxLat, double maxLon,
                                                     double originLat, double originLon, const QString &outputPath,
                                                     bool includeBuildings, bool includeGreenZones, bool includeWater, bool includeRailways) {
    m_pendingOutputPath = outputPath;
    m_pendingLat = originLat;
    m_pendingLon = originLon;
    m_pendingMinLat = minLat;
    m_pendingMinLon = minLon;
    m_pendingMaxLat = maxLat;
    m_pendingMaxLon = maxLon;
    m_pendingIncludeBuildings = includeBuildings;
    m_pendingIncludeGreenZones = includeGreenZones;
    m_pendingIncludeWater = includeWater;
    m_pendingIncludeRailways = includeRailways;

    m_currentEndpointIndex = 0;
    sendNextRequest();
}

void MapPreprocessor::sendNextRequest() {
    if (m_currentEndpointIndex >= m_apiUrls.size()) {
        QString errorMsg = "All configured Overpass API endpoints failed.";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        emit downloadFinished(false);
        return;
    }

    QString currentUrlStr = m_apiUrls[m_currentEndpointIndex];
    emit statusMessage(QString("Connecting To Server (Endpoint %1/%2)...").arg(m_currentEndpointIndex + 1).arg(m_apiUrls.size()));

    QStringList queryParts;
    queryParts.append("way[\"highway\"](%1,%2,%3,%4);");
    if (m_pendingIncludeRailways) queryParts.append("way[\"railway\"](%1,%2,%3,%4);");
    if (m_pendingIncludeWater) {
        queryParts.append("way[\"waterway\"](%1,%2,%3,%4);");
        queryParts.append("way[\"natural\"=\"water\"](%1,%2,%3,%4);");
    }
    if (m_pendingIncludeGreenZones) {
        queryParts.append("way[\"landuse\"~\"forest|grass|meadow|recreation_ground\"](%1,%2,%3,%4);");
        queryParts.append("way[\"leisure\"~\"park|nature_reserve\"](%1,%2,%3,%4);");
    }
    if (m_pendingIncludeBuildings) {
        queryParts.append("way[\"building\"](%1,%2,%3,%4);");
    }

    QString innerQuery = queryParts.join("");
    QString queryPayload = QString("[out:xml][timeout:120];(%1);(._;>;);out meta;")
                               .arg(innerQuery.arg(m_pendingMinLat).arg(m_pendingMinLon).arg(m_pendingMaxLat).arg(m_pendingMaxLon));

    QUrl targetUrl(currentUrlStr);
    QUrlQuery query;
    query.addQueryItem("data", queryPayload);
    targetUrl.setQuery(query);

    QNetworkRequest request(targetUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "BikeNetworkTool/1.0");

    m_networkManager.get(request);
}

void MapPreprocessor::onDownloadReply(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString warningMsg = QString("Endpoint %1 failed (%2). Trying next endpoint...")
        .arg(m_currentEndpointIndex + 1)
            .arg(reply->errorString());
        qWarning() << warningMsg;
        emit statusMessage(warningMsg);

        m_currentEndpointIndex++;
        sendNextRequest();
        return;
    }

    emit statusMessage("Storing Maps Temporarily...");

    QString tempPath = "temp_map_cache.osm";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        QString errorMsg = "Failed to initialize temporary disk storage maps.";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        emit downloadFinished(false);
        return;
    }

    tempFile.write(reply->readAll());
    tempFile.close();

    bool success = processOsmFile(tempPath, m_pendingOutputPath, m_pendingLat, m_pendingLon);
    QFile::remove(tempPath);

    emit downloadFinished(success);
}

bool MapPreprocessor::processOsmFile(const QString &inputPath, const QString &outputPath, double originLat, double originLon) {
    m_rawNodes.clear();
    m_rawFeatures.clear();
    m_projectedNodes.clear();

    emit statusMessage("Step 1/3: Parsing XML...");
    if (!parseXml(inputPath)) return false;

    emit statusMessage("Step 2/3: Projecting Grid...");
    projectCoordinates(originLat, originLon);

    emit statusMessage("Step 3/3: Serializing Data...");
    if (!writeBinaryFile(outputPath, originLat, originLon)) return false;

    emit statusMessage("Success! Map Compiled.");
    return true;
}

bool MapPreprocessor::parseXml(const QString &inputPath) {
    QFile file(inputPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString errorMsg = "Failed to open source OSM file.";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    QXmlStreamReader xml(&file);
    qint64 fileSize = file.size();

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (file.pos() % 500000 == 0 && fileSize > 0) {
            emit progressUpdated(static_cast<int>((file.pos() * 100) / fileSize));
        }

        if (token != QXmlStreamReader::StartElement) continue;

        if (xml.name() == QLatin1String("node")) {
            int64_t id = xml.attributes().value("id").toLongLong();
            double lat = xml.attributes().value("lat").toDouble();
            double lon = xml.attributes().value("lon").toDouble();
            m_rawNodes[id] = OsmNode{lat, lon};
        }
        else if (xml.name() == QLatin1String("way")) {
            OsmFeature currentFeature{0, 0, "", {}};

            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QLatin1String("way"))) {
                xml.readNext();
                if (xml.tokenType() != QXmlStreamReader::StartElement) continue;

                if (xml.name() == QLatin1String("nd")) {
                    currentFeature.nodeIds.append(xml.attributes().value("ref").toLongLong());
                }
                else if (xml.name() == QLatin1String("tag")) {
                    QString k = xml.attributes().value("k").toString();
                    QString v = xml.attributes().value("v").toString();

                    if (k == "highway") {
                        currentFeature.category = 1;
                        if (v == "cycleway" || v == "path" || v == "footway") currentFeature.subType = 1;
                        else if (v == "primary" || v == "secondary" || v == "tertiary") currentFeature.subType = 2;
                        else if (v == "residential" || v == "living_street" || v == "unclassified") currentFeature.subType = 3;
                        else if (v == "motorway" || v == "trunk") currentFeature.subType = 4;
                        else currentFeature.subType = 5;
                    }
                    else if (k == "railway") { currentFeature.category = 2; }
                    else if (k == "waterway") { currentFeature.category = 3; }
                    else if (k == "natural" && v == "water") { currentFeature.category = 4; }
                    else if ((k == "landuse" && (v == "forest" || v == "grass" || v == "meadow" || v == "recreation_ground")) ||
                             (k == "leisure" && (v == "park" || v == "nature_reserve"))) {
                        currentFeature.category = 5;
                    }
                    else if (k == "building") { currentFeature.category = 6; }
                    else if (k == "name") { currentFeature.name = v; }
                }
            }
            if (currentFeature.category > 0 && !currentFeature.nodeIds.isEmpty()) {
                m_rawFeatures.append(currentFeature);
            }
        }
    }

    if (xml.hasError()) {
        QString errorMsg = "XML parsing error: " + xml.errorString();
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
    return true;
}

QPointF MapPreprocessor::latLonToMetricOffsets(double lat, double lon, double originLat, double originLon) {
    const double EarthRadius = 6378137.0;

    double originX = EarthRadius * qDegreesToRadians(originLon);
    double originY = EarthRadius * qLn(qTan((90.0 + originLat) * M_PI / 360.0));

    double targetX = EarthRadius * qDegreesToRadians(lon);
    double targetY = EarthRadius * qLn(qTan((90.0 + lat) * M_PI / 360.0));

    double cosScale = qCos(qDegreesToRadians(originLat));

    return QPointF((targetX - originX) * cosScale, (targetY - originY) * cosScale);
}

void MapPreprocessor::projectCoordinates(double originLat, double originLon) {
    if (m_rawNodes.isEmpty()) return;

    int i = 0;
    for (auto it = m_rawNodes.begin(); it != m_rawNodes.end(); ++it, ++i) {
        if (i % 5000 == 0) {
            emit progressUpdated((i * 100) / m_rawNodes.size());
        }

        QPointF metricOffsets = latLonToMetricOffsets(it.value().lat, it.value().lon, originLat, originLon);
        m_projectedNodes[it.key()] = BinNode{
            static_cast<int32_t>(metricOffsets.x() * 1000.0),
            static_cast<int32_t>(metricOffsets.y() * 1000.0)
        };
    }
}

void MapPreprocessor::simplifyPolyline(QVector<BinNode> &nodes, double tolerance_mm) {
    if (nodes.size() <= 2) return;

    auto perpDist = [](const BinNode &p, const BinNode &a, const BinNode &b) {
        double dx = static_cast<double>(b.relX_mm - a.relX_mm);
        double dy = static_cast<double>(b.relY_mm - a.relY_mm);
        if (dx == 0.0 && dy == 0.0) {
            return qHypot(static_cast<double>(p.relX_mm - a.relX_mm), static_cast<double>(p.relY_mm - a.relY_mm));
        }
        double num = qAbs(dy * p.relX_mm - dx * p.relY_mm + b.relX_mm * a.relY_mm - b.relY_mm * a.relX_mm);
        return num / qHypot(dx, dy);
    };

    QVector<bool> keep(nodes.size(), true);
    std::function<void(int, int)> dp = [&](int first, int last) {
        if (last <= first + 1) return;
        double maxDist = 0.0;
        int index = first;
        for (int i = first + 1; i < last; ++i) {
            double dist = perpDist(nodes[i], nodes[first], nodes[last]);
            if (dist > maxDist) {
                maxDist = dist;
                index = i;
            }
        }
        if (maxDist > tolerance_mm) {
            dp(first, index);
            dp(index, last);
        } else {
            for (int i = first + 1; i < last; ++i) { keep[i] = false; }
        }
    };

    dp(0, nodes.size() - 1);
    QVector<BinNode> simplified;
    simplified.reserve(nodes.size());
    for (int i = 0; i < nodes.size(); ++i) {
        if (keep[i]) simplified.append(nodes[i]);
    }
    nodes = simplified;
}

bool MapPreprocessor::writeBinaryFile(const QString &outputPath, double originLat, double originLon) {
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QString errorMsg = "Could not write file to designated output path.";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    struct ValidFeature {
        uint8_t category;
        uint8_t subType;
        QString name;
        int32_t minX_mm, maxX_mm;
        int32_t minY_mm, maxY_mm;
        QVector<BinNode> nodes;
    };

    QVector<ValidFeature> validFeatures;
    for (const auto &feature : m_rawFeatures) {
        QVector<BinNode> validNodes;
        for (int64_t nodeId : feature.nodeIds) {
            if (m_projectedNodes.contains(nodeId)) {
                validNodes.append(m_projectedNodes.value(nodeId));
            }
        }

        if ((feature.category >= 4 && feature.category <= 6 && validNodes.size() >= 3) ||
            (feature.category < 4 && validNodes.size() >= 2)) {

            simplifyPolyline(validNodes, 1500.0);
            if (validNodes.isEmpty()) continue;

            int32_t minX_mm = 0x7FFFFFFF, maxX_mm = -0x7FFFFFFF;
            int32_t minY_mm = 0x7FFFFFFF, maxY_mm = -0x7FFFFFFF;
            for (const auto &node : validNodes) {
                minX_mm = qMin(minX_mm, node.relX_mm);
                maxX_mm = qMax(maxX_mm, node.relX_mm);
                minY_mm = qMin(minY_mm, node.relY_mm);
                maxY_mm = qMax(maxY_mm, node.relY_mm);
            }

            validFeatures.append(ValidFeature{feature.category, feature.subType, feature.name, minX_mm, maxX_mm, minY_mm, maxY_mm, validNodes});
        }
    }

    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    out.writeRawData("BMA2", 4);
    out << originLat << originLon;
    quint32 featureCount = static_cast<quint32>(validFeatures.size());
    out << featureCount;

    for (const auto &vf : validFeatures) {
        QByteArray nameUtf8 = vf.name.toUtf8();
        quint8 nameLen = static_cast<quint8>(qMin(nameUtf8.size(), 255));
        quint16 nodeCount = static_cast<quint16>(vf.nodes.size());

        out << vf.category << vf.subType << nodeCount
            << vf.minX_mm << vf.maxX_mm << vf.minY_mm << vf.maxY_mm
            << nameLen;

        if (nameLen > 0) {
            out.writeRawData(nameUtf8.constData(), nameLen);
        }

        for (const auto &bNode : vf.nodes) {
            out << bNode.relX_mm << bNode.relY_mm;
        }
    }

    outFile.close();
    emit progressUpdated(100);
    return true;
}