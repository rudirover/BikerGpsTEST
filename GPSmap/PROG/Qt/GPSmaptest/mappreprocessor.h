#pragma once

#include <QObject>
#include <QString>
#include <QXmlStreamReader>
#include <QHash>
#include <QPointF>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "mapdatastructs.h"

class MapPreprocessor : public QObject {
    Q_OBJECT

public:
    explicit MapPreprocessor(QObject *parent = nullptr);

    void downloadAndProcess(double originLat, double originLon, const QString &outputPath,
                            bool includeBuildings, bool includeGreenZones, bool includeWater, bool includeRailways);
    bool processOsmFile(const QString &inputPath, const QString &outputPath, double originLat, double originLon);

signals:
    void progressUpdated(int percentage);
    void statusMessage(const QString &message);
    void errorOccurred(const QString &error);
    void downloadFinished(bool success);

private slots:
    void onDownloadReply(QNetworkReply* reply);

private:
    struct OsmNode { double lat; double lon; };
    struct OsmFeature {
        uint8_t category;
        uint8_t subType;
        QString name;
        QVector<int64_t> nodeIds;
    };

    bool parseXml(const QString &inputPath);
    void projectCoordinates(double originLat, double originLon);
    bool writeBinaryFile(const QString &outputPath, double originLat, double originLon);
    void simplifyPolyline(QVector<BinNode> &nodes, double tolerance_mm);
    QPointF latLonToMetricOffsets(double lat, double lon, double originLat, double originLon);
    void calculateBoundingBox(double lat, double lon, double &minLat, double &minLon, double &maxLat, double &maxLon);

    QHash<int64_t, OsmNode> m_rawNodes;
    QVector<OsmFeature> m_rawFeatures;
    QHash<int64_t, BinNode> m_projectedNodes;

    QNetworkAccessManager* m_networkManager;
    QString m_pendingOutputPath;
    double m_pendingLat;
    double m_pendingLon;
};
