#pragma once

#include <QWidget>
#include <QImage>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include "mapdatastructs.h"

class MapCanvas : public QWidget {
    Q_OBJECT
public:
    explicit MapCanvas(QWidget *parent = nullptr);
    bool loadAndRenderBinaryMap(const QString &filePath);

    void setActiveStreetName(const QString &streetName);
    void setNavigationInstruction(const QString &instruction, const QString &distance);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(240, 320); }
    QSize minimumSizeHint() const override { return QSize(240, 320); }

private slots:
    void updateSimulationStep();

private:
    void renderMapFromData(double offsetX_mm, double offsetY_mm);

    QImage m_canvasImage;
    QByteArray m_mapData;
    QTimer *m_animTimer;

    double m_simOffsetX = 0.0;
    double m_simOffsetY = 0.0;
    double m_animAngle = 0.0;

    QString m_activeStreet = "Searching...";
    QString m_navInstruction = "Follow route";
    QString m_navDistance = "0 m";
};