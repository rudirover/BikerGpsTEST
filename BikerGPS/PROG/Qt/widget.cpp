#include "widget.hpp"
#include "ui_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressBar>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>

QSettings Widget::loadSettings() const {
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    return QSettings(configPath, QSettings::IniFormat);
}

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);

    QSettings settings = loadSettings();

    int winWidth = settings.value("UI/WindowWidth", 256).toInt();
    int winHeight = settings.value("UI/WindowHeight", 880).toInt();

    ui->verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
    this->setFixedHeight(winHeight);
    this->setFixedWidth(winWidth);

    m_bleController = new BleController(this);
    m_routeModel = new RouteModel(this);

    QString webUrl = settings.value("Web/MapViewerUrl", "https://www.fietsknooppunt.be/nl-be/").toString();
    QString webTitle = settings.value("Web/WindowTitle", "Fietsknooppunt Browser").toString();
    int webWidth = settings.value("Web/WindowWidth", 944).toInt();
    int webHeight = settings.value("Web/WindowHeight", 880).toInt();

    m_webView = new QWebEngineView();
    m_webView->setUrl(QUrl(webUrl));
    m_webView->setWindowTitle(webTitle);
    m_webView->resize(webWidth, webHeight);

    connect(m_webView->page()->profile(), &QWebEngineProfile::downloadRequested, this, &Widget::handleDownload);

    QProgressBar *progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setFormat("Idle");
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #3f3f46;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   background-color: #2a2a32;"
        "   color: white;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #10b981;"
        "   border-radius: 3px;"
        "}"
        );

    this->setProperty("progressBar", QVariant::fromValue(progressBar));
    ui->verticalLayout_2->addWidget(progressBar);

    connect(m_routeModel, &RouteModel::progressUpdated, this, [progressBar](int value) {
        progressBar->setValue(value);
    });

    connect(m_routeModel, &RouteModel::statusMessage, this, [progressBar](const QString &statusText) {
        progressBar->setFormat(statusText);
    });

    connect(m_routeModel, &RouteModel::errorOccurred, this, [this, progressBar](const QString &error) {
        progressBar->setValue(0);
        progressBar->setFormat("Error");
        QMessageBox::critical(this, "Route Error", error);
    });

    connect(m_bleController, &BleController::progressChanged, this, [progressBar](int value, const QString &statusText) {
        progressBar->setValue(value);
        progressBar->setFormat(statusText);
    });

    int resetDelay = settings.value("UI/ProgressResetDelayMs", 5000).toInt();
    connect(m_bleController, &BleController::transmissionFinished, this, [this, progressBar, resetDelay](bool) {
        ui->btnSend->setEnabled(true);
        QTimer::singleShot(resetDelay, this, [progressBar]() {
            progressBar->setValue(0);
            progressBar->setFormat("Idle");
        });
    });

    ui->btnSend->setEnabled(false); // Disable by default until map is compiled

    connect(m_routeModel, &RouteModel::mapCompilationFinished, this, [this](bool success) {
        ui->btnSend->setEnabled(success);
    });

    // Also disable it immediately when a new import/compilation starts
    connect(m_routeModel, &RouteModel::statusMessage, this, [this](const QString &text) {
        if (text.contains("Parsing") || text.contains("Opening")) {
            ui->btnSend->setEnabled(false);
        }
    });

    applyStyles();
}

Widget::~Widget() {
    if (m_webView) {
        m_webView->close();
        delete m_webView;
    }
    delete ui;
}

void Widget::handleDownload(QWebEngineDownloadRequest *download) {
    QSettings settings = loadSettings();
    QString filter = settings.value("UI/GpxDownloadFilter", "GPX Track (*.gpx);;All Files (*.*)").toString();

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                          + "/" + download->suggestedFileName();

    QString savePath = QFileDialog::getSaveFileName(m_webView, "Save Downloaded Track Data", defaultPath, filter);
    if (savePath.isEmpty()) {
        download->cancel();
        return;
    }

    QString targetDir = QFileInfo(savePath).absolutePath();
    QString targetName = QFileInfo(savePath).fileName();

    download->setDownloadDirectory(targetDir);
    download->setDownloadFileName(targetName);
    download->accept();

    QProgressBar *progressBar = this->property("progressBar").value<QProgressBar*>();
    if (progressBar) {
        progressBar->setFormat("Downloading Metadata...");
        progressBar->setValue(40);
    }

    QString fullDownloadedFilePath = QDir(targetDir).filePath(targetName);

    connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, download, progressBar, fullDownloadedFilePath]() {
        if (download->state() == QWebEngineDownloadRequest::DownloadCompleted) {
            if (progressBar) {
                progressBar->setValue(100);
                progressBar->setFormat("Importing Downloaded GPX...");
            }
            this->importGpxFile(fullDownloadedFilePath);
        } else if (download->state() == QWebEngineDownloadRequest::DownloadInterrupted) {
            QMessageBox::warning(m_webView, "Network Halt", "Web browser file download was canceled or interrupted.");
            if (progressBar) {
                progressBar->setValue(0);
                progressBar->setFormat("Idle");
            }
        }
    });
}

bool Widget::importGpxFile(const QString &fileName) {
    if (fileName.isEmpty()) return false;

    if (!m_routeModel->importFromGpx(fileName)) {
        QMessageBox::critical(this, "Error", "Could not import or parse selected GPX structure.");
        return false;
    }

    m_routeModel->compileMapFromImportedGpx();
    m_currentFileName = QFileInfo(fileName).fileName();

    updateVisualList();

    QSettings settings = loadSettings();
    int statusDelay = settings.value("UI/StatusMessageDelayMs", 3000).toInt();

    QProgressBar *progressBar = this->property("progressBar").value<QProgressBar*>();
    if (progressBar) {
        progressBar->setValue(100);
        progressBar->setFormat("Loaded " + QString::number(m_routeModel->nodeNumbers().count()) + " Nodes!");
        QTimer::singleShot(statusDelay, this, [progressBar]() {
            progressBar->setValue(0);
            progressBar->setFormat("Idle");
        });
    }
    return true;
}

void Widget::on_btnImport_clicked() {
    QSettings settings = loadSettings();
    QString filter = settings.value("UI/GpxOpenFilter", "GPX Files (*.gpx)").toString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open Fietsnet GPX", "", filter);
    importGpxFile(fileName);
}

void Widget::snapWebViewPosition() {
    if (!m_webView || !m_webView->isVisible()) return;

    QSettings settings = loadSettings();
    int snapOffset = settings.value("Web/SnapOffsetPx", 8).toInt();

    QRect mainFrame = this->frameGeometry();
    QRect webFrame = m_webView->frameGeometry();

    int titleBarDelta = (this->geometry().y() - mainFrame.y()) - (m_webView->geometry().y() - webFrame.y());
    int secondWindowX = mainFrame.x() + mainFrame.width() + snapOffset;
    int secondWindowY = mainFrame.y() + titleBarDelta;

    m_webView->move(secondWindowX, secondWindowY);
}

void Widget::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
    snapWebViewPosition();
}

void Widget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    snapWebViewPosition();
}

void Widget::updateVisualList() {
    ui->listWidget->clear();
    ui->listWidget->setUpdatesEnabled(false);

    // Optimized: Load settings ONCE outside the loop to prevent massive disk I/O and lag
    QSettings settings = loadSettings();
    QString separatorFormat = settings.value("UI/DistanceSeparatorFormat", "│  ▼  %1  ▼  │").toString();
    int itemWidth = settings.value("UI/ListItemWidth", 84).toInt();
    int itemHeight = settings.value("UI/ListItemHeight", 84).toInt();
    int distanceItemHeight = settings.value("UI/DistanceItemHeight", 35).toInt();
    QString distanceTextColor = settings.value("UI/DistanceTextColor", "#10b981").toString();

    int badgeSize = settings.value("UI/BadgeSize", 64).toInt();
    QString badgeBgColor = settings.value("UI/BadgeBgColor", "#23232a").toString();
    QString badgeBorderColor = settings.value("UI/BadgeBorderColor", "#10b981").toString();
    QString badgeTextColor = settings.value("UI/BadgeTextColor", "#ffffff").toString();
    int badgeBorderWidth = settings.value("UI/BadgeBorderWidth", 4).toInt();
    int badgeFontSize = settings.value("UI/BadgeFontSize", 24).toInt();

    int marginLeft = settings.value("UI/ContainerMarginLeft", 10).toInt();
    int marginTop = settings.value("UI/ContainerMarginTop", 4).toInt();
    int marginRight = settings.value("UI/ContainerMarginRight", 10).toInt();
    int marginBottom = settings.value("UI/ContainerMarginBottom", 4).toInt();

    QStringList nodes = m_routeModel->nodeNumbers();
    QStringList distances = m_routeModel->segmentDistances();

    for (int i = 0; i < nodes.count(); ++i) {
        addListItem(nodes[i]);

        if (i < nodes.count() - 1) {
            QListWidgetItem *distanceItem = new QListWidgetItem();
            distanceItem->setSizeHint(QSize(itemWidth, distanceItemHeight));
            ui->listWidget->addItem(distanceItem);

            QString finalDistanceText = (distances.size() > i) ? distances[i] : "?? km";

            QLabel *distanceLabel = new QLabel(separatorFormat.arg(finalDistanceText), this);
            distanceLabel->setAlignment(Qt::AlignCenter);
            distanceLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px; font-family: monospace; background: transparent;").arg(distanceTextColor));
            ui->listWidget->setItemWidget(distanceItem, distanceLabel);
        }
    }

    ui->listWidget->setUpdatesEnabled(true);
    ui->listWidget->doItemsLayout();
    ui->listWidget->update();
}

void Widget::on_btnSend_clicked() {
    if (m_routeModel->isEmpty()) {
        QMessageBox::information(this, "Empty Route", "There is no track configuration loaded to transmit.");
        return;
    }

    ui->btnSend->setEnabled(false);

    QSettings settings = loadSettings();
    QString bleDeviceName = settings.value("Bluetooth/DeviceName", "BikerNetworkTool").toString();
    QString jsonLogFile = settings.value("Route/JsonLogFilename", "sent_route_log.json").toString();
    QString mapLogFile = settings.value("Route/MapLogFilename", "sent_route_log.bma2").toString();

    // Extract base name from imported GPX file (e.g., "my_route.gpx" -> "my_route")
    QString baseName = "sent_route_log";
    if (!m_currentFileName.isEmpty()) {
        baseName = QFileInfo(m_currentFileName).completeBaseName();
    }

    // BLE metadata filenames use the GPX base name
    QString jsonBleName = baseName + ".json";
    QString mapBleName = baseName + ".bma2";

    QList<BleFilePayload> filesToSend;

    // Read local JSON file
    QFile jsonFile(jsonLogFile);
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        filesToSend.append({jsonBleName, jsonFile.readAll()});
    }

    // Read local BMA2 map file
    QFile mapFile(mapLogFile);
    if (mapFile.open(QIODevice::ReadOnly)) {
        filesToSend.append({mapBleName, mapFile.readAll()});
    }

    if (filesToSend.isEmpty()) {
        QMessageBox::warning(this, "Error", "Could not find generated log files to send.");
        ui->btnSend->setEnabled(true);
        return;
    }

    m_bleController->connectAndSendFiles(bleDeviceName, filesToSend);
}

void Widget::addListItem(const QString &text) {
    QSettings settings = loadSettings();

    int itemWidth = settings.value("UI/ListItemWidth", 84).toInt();
    int itemHeight = settings.value("UI/ListItemHeight", 84).toInt();
    int badgeSize = settings.value("UI/BadgeSize", 64).toInt();
    QString badgeBgColor = settings.value("UI/BadgeBgColor", "#23232a").toString();
    QString badgeBorderColor = settings.value("UI/BadgeBorderColor", "#10b981").toString();
    QString badgeTextColor = settings.value("UI/BadgeTextColor", "#ffffff").toString();
    int badgeBorderWidth = settings.value("UI/BadgeBorderWidth", 4).toInt();
    int badgeFontSize = settings.value("UI/BadgeFontSize", 24).toInt();

    int marginLeft = settings.value("UI/ContainerMarginLeft", 10).toInt();
    int marginTop = settings.value("UI/ContainerMarginTop", 4).toInt();
    int marginRight = settings.value("UI/ContainerMarginRight", 10).toInt();
    int marginBottom = settings.value("UI/ContainerMarginBottom", 4).toInt();

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(itemWidth, itemHeight));
    ui->listWidget->addItem(item);

    // Fixed: Removed the unmanaged memory leak from dead Widget allocation
    QWidget *baseContainer = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(baseContainer);
    layout->setContentsMargins(marginLeft, marginTop, marginRight, marginBottom);
    layout->addStretch();

    QLabel *circleBadge = new QLabel(text);
    circleBadge->setObjectName("circleLabel");
    circleBadge->setAlignment(Qt::AlignCenter);
    circleBadge->setFixedSize(badgeSize, badgeSize);
    circleBadge->setStyleSheet(QString("QLabel {"
                                       "   background-color: %1;"
                                       "   border: %2px solid %3;"
                                       "   border-radius: %4px;"
                                       "   color: %5;"
                                       "   font-size: %6px;"
                                       "   font-weight: bold;"
                                       "}")
                                   .arg(badgeBgColor)
                                   .arg(badgeBorderWidth)
                                   .arg(badgeBorderColor)
                                   .arg(badgeSize / 2)
                                   .arg(badgeTextColor)
                                   .arg(badgeFontSize));
    layout->addWidget(circleBadge, 0, Qt::AlignCenter);
    layout->addStretch();

    ui->listWidget->setItemWidget(item, baseContainer);
}

void Widget::applyStyles() {
    this->setStyleSheet(
        "QMainWindow { background-color: #1e1e24; }"
        "QListWidget {"
        "   background-color: #2a2a32;"
        "   border: 2px solid #3f3f46;"
        "   border-radius: 20px;"
        "   padding: 10px;"
        "   outline: 0; "
        "}"
        "QListWidget::item {"
        "   background: transparent;"
        "   margin-bottom: 4px;"
        "}"
        "QListWidget::item:hover {"
        "   background: #3a3a44;"
        "   border-radius: 8px;"
        "}"
        "QListWidget::item:selected {"
        "   background: #3f3f46;"
        "   border-radius: 8px;"
        "}"
        "QPushButton {"
        "   background-color: #3b82f6;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 10px 15px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }"
        );
}

void Widget::openWebUrl(const QString &urlStr) {
    if (m_webView->isVisible()) {
        m_webView->hide();
    }
    m_webView->setUrl(QUrl(urlStr));
    m_webView->show();
    snapWebViewPosition();
    m_webView->raise();
}

void Widget::on_btnOpen_clicked() {
    QSettings settings = loadSettings();
    QString webUrl = settings.value("Web/MapViewerUrl", "https://www.fietsknooppunt.be/nl-be/").toString();

    if (m_webView->isVisible() && m_webView->url().toString() == webUrl) {
        m_webView->hide();
    } else {
        openWebUrl(webUrl);
    }
}

void Widget::on_btnBunchies_clicked() {
    QSettings settings = loadSettings();
    QString targetUrl = settings.value("Web/BunchiesUrl", "https://www.bunchies.cc").toString();

    if (m_webView->isVisible() && m_webView->url().toString() == targetUrl) {
        m_webView->hide();
    } else {
        openWebUrl(targetUrl);
    }
}