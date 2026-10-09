#ifndef ROUTEMODEL_H
#define ROUTEMODEL_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QPair>
#include <QString>

class RouteModel : public QObject
{
    Q_OBJECT
public:
    explicit RouteModel(QObject *parent = nullptr);

    bool isEmpty() const;
    QJsonArray currentPathJson() const;
    QString toOrderedJsonString() const;
    QStringList nodeNumbers() const;
    QStringList segmentDistances() const;

    // Configuration I/O
    void loadConfig();

    // Helper & File I/O functions
    double getHaversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    bool importFromGpx(const QString &fileName);
    bool exportToGpx(const QString &fileName) const;
    bool saveRouteLogs() const;

private:
    QJsonArray m_currentPathJson;

    // Configurable tuning parameters
    double m_turnAngleThreshold = 45.0;
    double m_minDistBetweenWpts = 15.0;
    double m_sampleWindowMeters = 5.0;
    bool m_autoSaveLogs = true;
    int m_coordinatePrecision = 6;
    double m_earthRadiusMeters = 6371000.0;
    QString m_jsonLogFilename = "sent_route_log.json";
    QString m_gpxLogFilename = "sent_route_log.gpx";
    QString m_gpxCreator = "RouteModel";
    int m_startWaypointIndex = 1;
    QString m_gpxVersion = "1.1";
    QString m_gpxXmlns = "http://www.topografix.com/GPX/1/1";
    QString m_distanceUnit = "km";
    QString m_missingDistanceFallback = "?? km";
};

#endif // ROUTEMODEL_H