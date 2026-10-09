#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

class FietsnetImporter : public QObject {
    Q_OBJECT
public:
    explicit FietsnetImporter(QObject *parent = nullptr);
    ~FietsnetImporter();

    void fetchNodes(double minLat, double minLon, double maxLat, double maxLon);
    void loadConfig();

signals:
    void importStarted(const QString &statusText);
    void importProgress(int percentage);
    void importFinished(const QJsonArray &nodesArray);
    void importError(const QString &errorMessage);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    // Configurable Overpass API parameters loaded from config.ini
    QString m_apiUrl = "https://overpass-api.de/api/interpreter";
    int m_queryTimeoutSec = 120;
    QString m_userAgent = "BikeNetworkTool/1.0 (Contact: rudih@project.local)";
    double m_minSegmentDistanceMeters = 50.0;
    double m_earthRadiusMeters = 6371000.0;
    QString m_queryFilter = "relation[\"route\"=\"bicycle\"][\"network\"=\"rcn\"]";
    QString m_distanceUnit = "km";
};