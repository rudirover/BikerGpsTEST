#pragma once

#include <QWidget>
#include <QSettings>
#include "blecontroller.hpp"
#include "routemodel.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
class QWebEngineView;
class QWebEngineDownloadRequest;
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_btnSend_clicked();
    void on_btnImport_clicked();
    void on_btnOpen_clicked();
    void on_btnBunchies_clicked();
    void handleDownload(QWebEngineDownloadRequest *download);

private:
    Ui::Widget *ui;
    BleController *m_bleController;
    RouteModel *m_routeModel;
    QWebEngineView *m_webView;
    QString m_currentFileName;

    void openWebUrl(const QString &urlStr);
    bool importGpxFile(const QString &fileName);
    void addListItem(const QString &text); // Removed unused 'row' parameter
    void applyStyles();
    void updateVisualList();
    void snapWebViewPosition();
    QSettings loadSettings() const;
};