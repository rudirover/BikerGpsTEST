#pragma once

#include <QMainWindow>
#include "mappreprocessor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_browseInputButton_clicked();
    void on_browseOutputButton_clicked();
    void on_compileButton_clicked();

    // Core Background Worker UI Updates
    void handleProgress(int value);
    void handleStatus(const QString &msg);
    void handleError(const QString &err);

private:
    Ui::MainWindow *ui;
    MapPreprocessor *m_preprocessor;
};