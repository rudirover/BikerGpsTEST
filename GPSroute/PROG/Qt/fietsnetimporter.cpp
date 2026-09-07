#include "fietsnetimporter.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QtMath>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

FietsnetImporter::FietsnetImporter(QObject *parent) : QObject(parent) {
    loadConfig();
}

FietsnetImporter::~FietsnetImporter() {}

void FietsnetImporter::loadConfig() {
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    QSettings settings(configPath, QSettings::IniFormat);

    m_apiUrl = settings.value("Overpass/ApiUrl", "https://overpass-api.de/api/interpreter").toString();
    m_queryTimeoutSec = settings.value("Overpass/QueryTimeoutSec", 120).toInt();
    m_userAgent = settings.value("Overpass/UserAgent", "BikeNetworkTool/1.0 (Contact: rudih@project.local)").toString();
    m_minSegmentDistanceMeters = settings.value("Overpass/MinSegmentDistanceMeters", 50.0).toDouble();
    m_earthRadiusMeters = settings.value("Overpass/EarthRadiusMeters", 6371000.0).toDouble();
    m_queryFilter = settings.value("Overpass/QueryFilter", "relation[\"route\"=\"bicycle\"][\"network\"=\"rcn\"]").toString();
    m_distanceUnit = settings.value("Overpass/DistanceUnit", "km").toString();
}

void FietsnetImporter::fetchNodes(double minLat, double minLon, double maxLat, double maxLon) {
    loadConfig();
    auto *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &FietsnetImporter::onReplyFinished);

    emit importStarted("Fetching network routes and distances...");
    emit importProgress(15);

    QString overpassQuery = QString(
                                "[out:json][timeout:%1];"
                                "%2(%3,%4,%5,%6);"
                                "out geom;"
                                ).arg(QString::number(m_queryTimeoutSec),
                                     m_queryFilter,
                                     QString::number(minLat), QString::number(minLon),
                                     QString::number(maxLat), QString::number(maxLon));

    QUrl url(m_apiUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());

    QByteArray postData = "data=" + QUrl::toPercentEncoding(overpassQuery.toUtf8());
    manager->post(request, postData);
}

static double calculateHaversine(double lat1, double lon1, double lat2, double lon2, double radius) {
    double dLat = qDegreesToRadians(lat2 - lat1);
    double dLon = qDegreesToRadians(lon2 - lon1);

    double rLat1 = qDegreesToRadians(lat1);
    double rLat2 = qDegreesToRadians(lat2);

    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(rLat1) * std::cos(rLat2) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return radius * c;
}

void FietsnetImporter::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();
    reply->manager()->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit importError("Download failed: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject rootObj = doc.object();
    QJsonArray elements = rootObj["elements"].toArray();

    QMap<QString, QList<double>> routeDistanceMap;
    QMap<QString, QPair<QString, QString>> nodePairMap;

    for (const QJsonValue &val : elements) {
        QJsonObject element = val.toObject();
        if (element["type"].toString() == "relation") {
            QJsonObject tags = element["tags"].toObject();

            QString fromNode = "";
            QString toNode = "";

            QString ref = tags["ref"].toString();
            if (ref.contains("-")) {
                QStringList parts = ref.split("-");
                fromNode = parts.first().trimmed();
                toNode = parts.last().trimmed();
            } else if (tags.contains("from") && tags.contains("to")) {
                fromNode = tags["from"].toString().trimmed();
                toNode = tags["to"].toString().trimmed();
            } else {
                QString note = tags["note"].toString();
                if (note.isEmpty()) note = tags["name"].toString();

                if (note.contains("-")) {
                    QStringList parts = note.split("-");
                    fromNode = parts.first().trimmed();
                    toNode = parts.last().trimmed();
                }
            }

            if (!fromNode.isEmpty() && !toNode.isEmpty()) {
                double totalDistance = 0.0;
                QJsonArray members = element["members"].toArray();

                for (const QJsonValue &memberVal : members) {
                    QJsonObject member = memberVal.toObject();
                    if (member.contains("geometry")) {
                        QJsonArray geometry = member["geometry"].toArray();
                        for (int i = 0; i < geometry.count() - 1; ++i) {
                            QJsonObject pt1 = geometry[i].toObject();
                            QJsonObject pt2 = geometry[i+1].toObject();
                            totalDistance += calculateHaversine(pt1["lat"].toDouble(), pt1["lon"].toDouble(),
                                                                pt2["lat"].toDouble(), pt2["lon"].toDouble(),
                                                                m_earthRadiusMeters);
                        }
                    }
                }

                if (totalDistance > m_minSegmentDistanceMeters) {
                    QString firstNode = (fromNode.toInt() < toNode.toInt()) ? fromNode : toNode;
                    QString secondNode = (fromNode.toInt() < toNode.toInt()) ? toNode : fromNode;
                    QString uniqueKey = firstNode + "_" + secondNode;

                    routeDistanceMap[uniqueKey].append(totalDistance);
                    nodePairMap[uniqueKey] = qMakePair(fromNode, toNode);
                }
            }
        }
    }

    QJsonArray structuredNetwork;
    for (auto it = routeDistanceMap.constBegin(); it != routeDistanceMap.constEnd(); ++it) {
        QString key = it.key();
        QList<double> distances = it.value();
        QPair<QString, QString> nodes = nodePairMap[key];

        double sum = 0;
        for (double d : distances) sum += d;
        double finalDistance = sum / distances.count();

        QJsonObject routeSegment;
        routeSegment["from"] = nodes.first;
        routeSegment["to"] = nodes.second;
        routeSegment["distance"] = QString::number(finalDistance / 1000.0, 'f', 1) + " " + m_distanceUnit;
        structuredNetwork.append(routeSegment);
    }

    emit importProgress(100);
    emit importFinished(structuredNetwork);
}