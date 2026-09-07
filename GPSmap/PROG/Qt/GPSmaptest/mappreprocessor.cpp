#include "mappreprocessor.h"
#include <QFile>
#include <QUrl>
#include <QNetworkRequest>
#include <QtMath>
#include <QNetworkProxy>
#include <QDebug>
#include <QUrlQuery>
#include <cstring>
#include <functional>

MapPreprocessor::MapPreprocessor(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this))
{
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    connect(m_networkManager, &QNetworkAccessManager::finished, this, &MapPreprocessor::onDownloadReply);
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
    m_pendingOutputPath = outputPath;
    m_pendingLat = originLat;
    m_pendingLon = originLon;

    double minLat, minLon, maxLat, maxLon;
    calculateBoundingBox(originLat, originLon, minLat, minLon, maxLat, maxLon);

    QStringList queryParts;
    queryParts.append("way[\"highway\"](%1,%2,%3,%4);");
    if (includeRailways) queryParts.append("way[\"railway\"](%1,%2,%3,%4);");
    if (includeWater) {
        queryParts.append("way[\"waterway\"](%1,%2,%3,%4);");
        queryParts.append("way[\"natural\"=\"water\"](%1,%2,%3,%4);");
    }
    if (includeGreenZones) {
        queryParts.append("way[\"landuse\"~\"forest|grass|meadow|recreation_ground\"](%1,%2,%3,%4);");
        queryParts.append("way[\"leisure\"~\"park|nature_reserve\"](%1,%2,%3,%4);");
    }
    if (includeBuildings) {
        queryParts.append("way[\"building\"](%1,%2,%3,%4);");
    }

    QString innerQuery = queryParts.join("");
    QString queryPayload = QString("[out:xml][timeout:90];(%1);(._;>;);out meta;")
                               .arg(innerQuery.arg(minLat).arg(minLon).arg(maxLat).arg(maxLon));

    QUrl targetUrl("https://overpass-api.de/api/interpreter");
    QUrlQuery query;
    query.addQueryItem("data", queryPayload);
    targetUrl.setQuery(query);

    emit statusMessage(QString("Connecting to Overpass Server for [%1, %2]...").arg(originLat).arg(originLon));

    QNetworkRequest request(targetUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ESP32BikeMapCompiler/1.0");

    m_networkManager->get(request);
}

void MapPreprocessor::onDownloadReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Network error downloading map: " + reply->errorString();
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        emit downloadFinished(false);
        reply->deleteLater();
        return;
    }

    emit statusMessage("Download completed! Storing maps temporarily...");

    QString tempPath = "temp_map_cache.osm";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        QString errorMsg = "Failed to initialize temporary disk storage maps.";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        emit downloadFinished(false);
        reply->deleteLater();
        return;
    }

    tempFile.write(reply->readAll());
    tempFile.close();
    reply->deleteLater();

    bool success = processOsmFile(tempPath, m_pendingOutputPath, m_pendingLat, m_pendingLon);
    QFile::remove(tempPath);

    emit downloadFinished(success);
}

bool MapPreprocessor::processOsmFile(const QString &inputPath, const QString &outputPath, double originLat, double originLon) {
    m_rawNodes.clear();
    m_rawFeatures.clear();
    m_projectedNodes.clear();

    emit statusMessage("Step 1/3: Parsing OpenStreetMap XML data...");
    if (!parseXml(inputPath)) return false;

    emit statusMessage("Step 2/3: Projecting coordinates into metric grid...");
    projectCoordinates(originLat, originLon);

    emit statusMessage("Step 3/3: Serializing data into ESP32 Binary Format...");
    if (!writeBinaryFile(outputPath, originLat, originLon)) return false;

    emit statusMessage("Success! Map compiled successfully.");
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

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == QLatin1String("node")) {
                int64_t id = xml.attributes().value("id").toLongLong();
                double lat = xml.attributes().value("lat").toDouble();
                double lon = xml.attributes().value("lon").toDouble();
                m_rawNodes[id] = OsmNode{lat, lon};
            }
            else if (xml.name() == QLatin1String("way")) {
                OsmFeature currentFeature;
                currentFeature.category = 0;
                currentFeature.subType = 0;
                currentFeature.name = "";

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QLatin1String("way"))) {
                    xml.readNext();
                    if (xml.tokenType() == QXmlStreamReader::StartElement) {
                        if (xml.name() == QLatin1String("nd")) {
                            int64_t refId = xml.attributes().value("ref").toLongLong();
                            currentFeature.nodeIds.append(refId);
                        }
                        else if (xml.name() == QLatin1String("tag")) {
                            QString k = xml.attributes().value("k").toString();
                            QString v = xml.attributes().value("v").toString();

                            if (k == "highway") {
                                currentFeature.category = 1;
                                if (v == "cycleway" || v == "path" || v == "footway") {
                                    currentFeature.subType = 1;
                                } else if (v == "primary" || v == "secondary" || v == "tertiary") {
                                    currentFeature.subType = 2;
                                } else if (v == "residential" || v == "living_street" || v == "unclassified") {
                                    currentFeature.subType = 3;
                                } else if (v == "motorway" || v == "trunk") {
                                    currentFeature.subType = 4;
                                } else {
                                    currentFeature.subType = 5;
                                }
                            }
                            else if (k == "railway") {
                                currentFeature.category = 2;
                                currentFeature.subType = 0;
                            }
                            else if (k == "waterway") {
                                currentFeature.category = 3;
                                currentFeature.subType = 0;
                            }
                            else if (k == "natural" && v == "water") {
                                currentFeature.category = 4;
                                currentFeature.subType = 0;
                            }
                            else if ((k == "landuse" && (v == "forest" || v == "grass" || v == "meadow" || v == "recreation_ground")) ||
                                     (k == "leisure" && (v == "park" || v == "nature_reserve"))) {
                                currentFeature.category = 5;
                                currentFeature.subType = 0;
                            }
                            else if (k == "building") {
                                currentFeature.category = 6;
                                currentFeature.subType = 0;
                            }
                            else if (k == "name") {
                                currentFeature.name = v;
                            }
                        }
                    }
                }
                if (currentFeature.category > 0 && !currentFeature.nodeIds.isEmpty()) {
                    m_rawFeatures.append(currentFeature);
                }
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

    double deltaX = (targetX - originX) * cosScale;
    double deltaY = (targetY - originY) * cosScale;

    return QPointF(deltaX, deltaY);
}

void MapPreprocessor::projectCoordinates(double originLat, double originLon) {
    int i = 0;
    if (m_rawNodes.isEmpty()) return;

    for (auto it = m_rawNodes.begin(); it != m_rawNodes.end(); ++it) {
        if (i++ % 5000 == 0) {
            emit progressUpdated((i * 100) / m_rawNodes.size());
        }

        QPointF metricOffsets = latLonToMetricOffsets(it.value().lat, it.value().lon, originLat, originLon);

        BinNode bNode;
        bNode.relX_mm = static_cast<int32_t>(metricOffsets.x() * 1000.0);
        bNode.relY_mm = static_cast<int32_t>(metricOffsets.y() * 1000.0);
        m_projectedNodes[it.key()] = bNode;
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
        double den = qHypot(dx, dy);
        return num / den;
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
            for (int i = first + 1; i < last; ++i) {
                keep[i] = false;
            }
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
        int32_t minX_mm;
        int32_t maxX_mm;
        int32_t minY_mm;
        int32_t maxY_mm;
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
                if (node.relX_mm < minX_mm) minX_mm = node.relX_mm;
                if (node.relX_mm > maxX_mm) maxX_mm = node.relX_mm;
                if (node.relY_mm < minY_mm) minY_mm = node.relY_mm;
                if (node.relY_mm > maxY_mm) maxY_mm = node.relY_mm;
            }

            validFeatures.append(ValidFeature{feature.category, feature.subType, feature.name, minX_mm, maxX_mm, minY_mm, maxY_mm, validNodes});
        }
    }

    // Write V2 header ("BMA2") with precomputed bounding box support
    outFile.write("BMA2", 4);
    outFile.write(reinterpret_cast<const char*>(&originLat), sizeof(double));
    outFile.write(reinterpret_cast<const char*>(&originLon), sizeof(double));
    uint32_t featureCount = static_cast<uint32_t>(validFeatures.size());
    outFile.write(reinterpret_cast<const char*>(&featureCount), sizeof(uint32_t));

    for (const auto &vf : validFeatures) {
        QByteArray nameUtf8 = vf.name.toUtf8();
        uint8_t nameLen = static_cast<uint8_t>(qMin(nameUtf8.size(), 255));

        outFile.write(reinterpret_cast<const char*>(&vf.category), 1);
        outFile.write(reinterpret_cast<const char*>(&vf.subType), 1);

        uint16_t nodeCount = static_cast<uint16_t>(vf.nodes.size());
        outFile.write(reinterpret_cast<const char*>(&nodeCount), 2);

        outFile.write(reinterpret_cast<const char*>(&vf.minX_mm), 4);
        outFile.write(reinterpret_cast<const char*>(&vf.maxX_mm), 4);
        outFile.write(reinterpret_cast<const char*>(&vf.minY_mm), 4);
        outFile.write(reinterpret_cast<const char*>(&vf.maxY_mm), 4);

        outFile.write(reinterpret_cast<const char*>(&nameLen), 1);

        if (nameLen > 0) {
            outFile.write(nameUtf8.constData(), nameLen);
        }

        for (const auto &bNode : vf.nodes) {
            outFile.write(reinterpret_cast<const char*>(&bNode.relX_mm), 4);
            outFile.write(reinterpret_cast<const char*>(&bNode.relY_mm), 4);
        }
    }

    outFile.close();
    emit progressUpdated(100);
    return true;
}

