#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include "mapcanvas.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_preprocessor(new MapPreprocessor(this))
{
    ui->setupUi(this);

    connect(m_preprocessor, &MapPreprocessor::progressUpdated, this, &MainWindow::handleProgress);
    connect(m_preprocessor, &MapPreprocessor::statusMessage, this, &MainWindow::handleStatus);
    connect(m_preprocessor, &MapPreprocessor::errorOccurred, this, &MainWindow::handleError);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_browseInputButton_clicked() {
    // Left empty since we download files over the network dynamically
}

void MainWindow::on_browseOutputButton_clicked() {
    QString path = QFileDialog::getSaveFileName(this, "Save Packed Binary Asset File", "", "Map Binary (*.bin)");
    if (!path.isEmpty()) {
        ui->outputPathEdit->setText(path);
    }
}

void MainWindow::on_compileButton_clicked() {
    QString outPath = ui->outputPathEdit->text();
    double originLat = ui->latSpinBox->value();
    double originLon = ui->lonSpinBox->value();

    bool incBuildings = ui->chkBuildings->isChecked();
    bool incGreenZones = ui->chkGreenZones->isChecked();
    bool incWater = ui->chkWater->isChecked();
    bool incRailways = ui->chkRailways->isChecked();

    if (outPath.isEmpty()) {
        QMessageBox::warning(this, "Output Path Missing", "Please assign an output path to save the .bin asset.");
        return;
    }

    ui->compileButton->setEnabled(false);

    connect(m_preprocessor, &MapPreprocessor::downloadFinished, this, [this, outPath](bool success){
        ui->compileButton->setEnabled(true);
        if(success) {
            ui->statusbar->showMessage("Processing fully complete!");

            MapCanvas *canvas = new MapCanvas(nullptr);
            canvas->setAttribute(Qt::WA_DeleteOnClose);
            canvas->setWindowTitle("ESP32 Display Simulation (320x240)");
            canvas->show();
            canvas->loadAndRenderBinaryMap(outPath);
        }
    });

    m_preprocessor->downloadAndProcess(originLat, originLon, outPath, incBuildings, incGreenZones, incWater, incRailways);
}

void MainWindow::handleProgress(int value) {
    ui->progressBar->setValue(value);
}

void MainWindow::handleStatus(const QString &msg) {
    ui->statusbar->showMessage(msg);
}

void MainWindow::handleError(const QString &err) {
    QMessageBox::critical(this, "Processing Error Encountered", err);
}
