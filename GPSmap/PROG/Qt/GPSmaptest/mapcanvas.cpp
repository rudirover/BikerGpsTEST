#include "mapcanvas.h"
#include <QFile>
#include <QPainter>
#include <QDebug>
#include <QtMath>

MapCanvas::MapCanvas(QWidget *parent)
    : QWidget(parent), m_canvasImage(240, 320, QImage::Format_RGB32), m_animTimer(new QTimer(this)) {
    m_canvasImage.fill(QColor(242, 239, 233));
    setFixedSize(240, 320);

    connect(m_animTimer, &QTimer::timeout, this, &MapCanvas::updateSimulationStep);
}

void MapCanvas::setActiveStreetName(const QString &streetName) {
    m_activeStreet = streetName;
    update();
}

void MapCanvas::setNavigationInstruction(const QString &instruction, const QString &distance) {
    m_navInstruction = instruction;
    m_navDistance = distance;
    update();
}

void MapCanvas::updateSimulationStep() {
    m_animAngle += 0.002;
    m_simOffsetX = 900000.0 * qCos(m_animAngle);
    m_simOffsetY = 900000.0 * qSin(m_animAngle);

    if (!m_mapData.isEmpty()) {
        renderMapFromData(m_simOffsetX, m_simOffsetY);
    }
}

void MapCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.drawImage(0, 0, m_canvasImage);

    QRect bannerRect(0, height() - 55, width(), 55);
    painter.fillRect(bannerRect, QColor(30, 35, 45, 230));
    painter.setPen(QPen(QColor(80, 90, 110), 1));
    painter.drawRect(bannerRect.adjusted(0, 0, 0, -1));

    QRect textRect(10, height() - 50, width() - 20, 45);
    painter.setPen(Qt::white);

    QFont instrFont = painter.font();
    instrFont.setPointSize(9);
    instrFont.setBold(true);
    painter.setFont(instrFont);
    painter.drawText(textRect.adjusted(5, 2, -5, -24), Qt::AlignLeft | Qt::AlignVCenter, "Where am I?");

    QFont streetFont = painter.font();
    streetFont.setPointSize(8);
    streetFont.setBold(false);
    painter.setFont(streetFont);
    painter.setPen(QColor(180, 190, 205));
    painter.drawText(textRect.adjusted(5, 22, -5, -2), Qt::AlignLeft | Qt::AlignVCenter, m_activeStreet);

    painter.setPen(QPen(Qt::gray, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, width(), height() - 1);
}

bool MapCanvas::loadAndRenderBinaryMap(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open binary map file for rendering:" << filePath;
        return false;
    }

    m_mapData = file.readAll();
    file.close();

    renderMapFromData(0.0, 0.0);

    m_animTimer->start(50);
    return true;
}

void MapCanvas::renderMapFromData(double offsetX_mm, double offsetY_mm) {
    if (m_mapData.isEmpty()) return;

    const char *ptr = m_mapData.constData();
    qint64 totalSize = m_mapData.size();

    if (totalSize < static_cast<qint64>(sizeof(BinHeader))) return;

    BinHeader header;
    std::memcpy(&header, ptr, sizeof(BinHeader));
    ptr += sizeof(BinHeader);

    bool isV2Format = false;
    if (std::memcmp(header.magic, "BMA2", 4) == 0) {
        isV2Format = true;
    } else if (std::memcmp(header.magic, "BMAP", 4) != 0) {
        return;
    }

    m_canvasImage.fill(QColor(242, 239, 233));

    QPainter painter(&m_canvasImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const float screenW = 240.0f;
    const float screenH = 320.0f;
    const float centerX = screenW / 2.0f;
    const float centerY = screenH / 2.0f;

    float maxExtentMm = 50000.0f;
    float scale = (screenW / 2.0f) / maxExtentMm;

    QString nearestStreet = "";
    double minCenterDist_mm = 20000.0;

    for (uint32_t r = 0; r < header.featureCount; ++r) {
        uint8_t category, subType;
        uint16_t nodeCount;
        uint8_t nameLength = 0;
        QString featureName = "";
        bool visible = false;

        int32_t minX_mm = 0, maxX_mm = 0, minY_mm = 0, maxY_mm = 0;

        if (isV2Format) {
            if (ptr - m_mapData.constData() + static_cast<ptrdiff_t>(sizeof(BinFeatureV2)) > totalSize) break;

            BinFeatureV2 feature;
            std::memcpy(&feature, ptr, sizeof(BinFeatureV2));
            ptr += sizeof(BinFeatureV2);

            category = feature.category;
            subType = feature.subType;
            nodeCount = feature.nodeCount;
            minX_mm = feature.minX_mm;
            maxX_mm = feature.maxX_mm;
            minY_mm = feature.minY_mm;
            maxY_mm = feature.maxY_mm;
            nameLength = feature.nameLength;

            if (nameLength > 0) {
                if (ptr - m_mapData.constData() + nameLength > totalSize) break;
                featureName = QString::fromUtf8(ptr, nameLength);
                ptr += nameLength;
            }

            if (nodeCount == 0 || nodeCount > 5000) break;
            if (ptr - m_mapData.constData() + nodeCount * static_cast<ptrdiff_t>(sizeof(BinNode)) > totalSize) break;

            // Instant precomputed bounding box viewport check
            int32_t spanX = static_cast<int32_t>(maxExtentMm * 1.5f);
            int32_t spanY = static_cast<int32_t>(maxExtentMm * 1.5f * (screenH / screenW));

            if (maxX_mm >= offsetX_mm - spanX && minX_mm <= offsetX_mm + spanX &&
                maxY_mm >= offsetY_mm - spanY && minY_mm <= offsetY_mm + spanY) {
                visible = true;
            }

            if (!visible && category != 1) {
                ptr += nodeCount * sizeof(BinNode);
                continue;
            }
        } else {
            // Legacy Fallback ("BMAP") using node-by-node inspection
            if (ptr - m_mapData.constData() + static_cast<ptrdiff_t>(sizeof(BinFeatureLegacy)) > totalSize) break;

            BinFeatureLegacy feature;
            std::memcpy(&feature, ptr, sizeof(BinFeatureLegacy));
            ptr += sizeof(BinFeatureLegacy);

            category = feature.category;
            subType = feature.subType;
            nodeCount = feature.nodeCount;
            nameLength = feature.nameLength;

            if (nameLength > 0) {
                if (ptr - m_mapData.constData() + nameLength > totalSize) break;
                featureName = QString::fromUtf8(ptr, nameLength);
                ptr += nameLength;
            }

            if (nodeCount == 0 || nodeCount > 5000) break;
            if (ptr - m_mapData.constData() + nodeCount * static_cast<ptrdiff_t>(sizeof(BinNode)) > totalSize) break;

            int32_t minPx = 9999, maxPx = -9999, minPy = 9999, maxPy = -9999;
            const char *nodeCheckPtr = ptr;

            for (uint16_t n = 0; n < nodeCount; ++n) {
                BinNode node;
                std::memcpy(&node, nodeCheckPtr, sizeof(BinNode));
                nodeCheckPtr += sizeof(BinNode);

                float adjustedX = static_cast<float>(node.relX_mm) - static_cast<float>(offsetX_mm);
                float adjustedY = static_cast<float>(node.relY_mm) - static_cast<float>(offsetY_mm);

                float px = centerX + (adjustedX * scale);
                float py = centerY - (adjustedY * scale);

                if (px < minPx) minPx = px;
                if (px > maxPx) maxPx = px;
                if (py < minPy) minPy = py;
                if (py > maxPy) maxPy = py;
            }

            if (maxPx >= -50 && minPx <= screenW + 50 && maxPy >= -50 && minPy <= screenH + 50) {
                visible = true;
            }

            if (!visible && category != 1) {
                ptr += nodeCount * sizeof(BinNode);
                continue;
            }
        }

        QVector<QPointF> screenPoints;
        screenPoints.reserve(nodeCount);

        for (uint16_t n = 0; n < nodeCount; ++n) {
            BinNode node;
            std::memcpy(&node, ptr, sizeof(BinNode));
            ptr += sizeof(BinNode);

            float adjustedX = static_cast<float>(node.relX_mm) - static_cast<float>(offsetX_mm);
            float adjustedY = static_cast<float>(node.relY_mm) - static_cast<float>(offsetY_mm);

            if (category == 1 && !featureName.isEmpty()) {
                double dist_mm = qHypot(adjustedX, adjustedY);
                if (dist_mm < minCenterDist_mm) {
                    minCenterDist_mm = dist_mm;
                    nearestStreet = featureName;
                }
            }

            float px = centerX + (adjustedX * scale);
            float py = centerY - (adjustedY * scale);

            screenPoints.append(QPointF(px, py));
        }

        if (!visible) continue;

        if (screenPoints.size() > 2 && (category >= 4 && category <= 6)) {
            QColor fillColor, borderColor;
            if (category == 4) {
                fillColor = QColor(181, 208, 208);
                borderColor = QColor(140, 175, 175);
            } else if (category == 5) {
                fillColor = QColor(200, 235, 195);
                borderColor = QColor(170, 210, 165);
            } else {
                fillColor = QColor(229, 219, 211);
                borderColor = QColor(199, 189, 181);
            }

            painter.setBrush(QBrush(fillColor));
            painter.setPen(QPen(borderColor, 1));
            painter.drawPolygon(screenPoints.constData(), screenPoints.size());
        }
        else if (screenPoints.size() > 1) {
            QPen pen;
            if (category == 1) {
                if (subType == 1) {
                    pen = QPen(QColor(130, 190, 110), 1, Qt::DashLine);
                } else if (subType == 2) {
                    pen = QPen(QColor(245, 160, 80), 3);
                } else if (subType == 3) {
                    pen = QPen(QColor(255, 255, 255), 2);
                } else if (subType == 4) {
                    pen = QPen(QColor(232, 107, 86), 5);
                } else {
                    pen = QPen(QColor(210, 210, 210), 1);
                }
            } else if (category == 2) {
                pen = QPen(Qt::darkGray, 1, Qt::DashLine);
            } else if (category == 3) {
                pen = QPen(QColor(135, 180, 220), 2);
            } else {
                pen = QPen(QColor(180, 180, 180), 1);
            }

            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(screenPoints.constData(), screenPoints.size());
        }
    }

    setActiveStreetName(nearestStreet);

    painter.setPen(QPen(QColor(210, 40, 40), 1, Qt::DashLine));
    painter.drawLine(centerX - 5, centerY, centerX + 5, centerY);
    painter.drawLine(centerX, centerY - 5, centerX, centerY + 5);

    update();
}
