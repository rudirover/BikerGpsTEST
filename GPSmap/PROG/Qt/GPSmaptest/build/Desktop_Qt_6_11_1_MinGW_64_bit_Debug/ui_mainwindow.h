/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayoutFiles;
    QLabel *labelInput;
    QLineEdit *inputPathEdit;
    QPushButton *browseInputButton;
    QLabel *labelOutput;
    QLineEdit *outputPathEdit;
    QPushButton *browseOutputButton;
    QGroupBox *groupBoxCoordinates;
    QHBoxLayout *horizontalLayoutCoords;
    QLabel *labelLat;
    QDoubleSpinBox *latSpinBox;
    QLabel *labelLon;
    QDoubleSpinBox *lonSpinBox;
    QGroupBox *groupBoxLayers;
    QHBoxLayout *horizontalLayoutLayers;
    QCheckBox *chkBuildings;
    QCheckBox *chkGreenZones;
    QCheckBox *chkWater;
    QCheckBox *chkRailways;
    QSpacerItem *verticalSpacer;
    QProgressBar *progressBar;
    QPushButton *compileButton;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(500, 420);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(12);
        verticalLayout->setObjectName("verticalLayout");
        gridLayoutFiles = new QGridLayout();
        gridLayoutFiles->setSpacing(8);
        gridLayoutFiles->setObjectName("gridLayoutFiles");
        labelInput = new QLabel(centralwidget);
        labelInput->setObjectName("labelInput");

        gridLayoutFiles->addWidget(labelInput, 0, 0, 1, 1);

        inputPathEdit = new QLineEdit(centralwidget);
        inputPathEdit->setObjectName("inputPathEdit");

        gridLayoutFiles->addWidget(inputPathEdit, 0, 1, 1, 1);

        browseInputButton = new QPushButton(centralwidget);
        browseInputButton->setObjectName("browseInputButton");

        gridLayoutFiles->addWidget(browseInputButton, 0, 2, 1, 1);

        labelOutput = new QLabel(centralwidget);
        labelOutput->setObjectName("labelOutput");

        gridLayoutFiles->addWidget(labelOutput, 1, 0, 1, 1);

        outputPathEdit = new QLineEdit(centralwidget);
        outputPathEdit->setObjectName("outputPathEdit");

        gridLayoutFiles->addWidget(outputPathEdit, 1, 1, 1, 1);

        browseOutputButton = new QPushButton(centralwidget);
        browseOutputButton->setObjectName("browseOutputButton");

        gridLayoutFiles->addWidget(browseOutputButton, 1, 2, 1, 1);


        verticalLayout->addLayout(gridLayoutFiles);

        groupBoxCoordinates = new QGroupBox(centralwidget);
        groupBoxCoordinates->setObjectName("groupBoxCoordinates");
        horizontalLayoutCoords = new QHBoxLayout(groupBoxCoordinates);
        horizontalLayoutCoords->setSpacing(15);
        horizontalLayoutCoords->setObjectName("horizontalLayoutCoords");
        labelLat = new QLabel(groupBoxCoordinates);
        labelLat->setObjectName("labelLat");

        horizontalLayoutCoords->addWidget(labelLat);

        latSpinBox = new QDoubleSpinBox(groupBoxCoordinates);
        latSpinBox->setObjectName("latSpinBox");
        latSpinBox->setDecimals(6);
        latSpinBox->setMinimum(-90.000000000000000);
        latSpinBox->setMaximum(90.000000000000000);
        latSpinBox->setValue(51.164499999999997);

        horizontalLayoutCoords->addWidget(latSpinBox);

        labelLon = new QLabel(groupBoxCoordinates);
        labelLon->setObjectName("labelLon");

        horizontalLayoutCoords->addWidget(labelLon);

        lonSpinBox = new QDoubleSpinBox(groupBoxCoordinates);
        lonSpinBox->setObjectName("lonSpinBox");
        lonSpinBox->setDecimals(6);
        lonSpinBox->setMinimum(-180.000000000000000);
        lonSpinBox->setMaximum(180.000000000000000);
        lonSpinBox->setValue(4.139600000000000);

        horizontalLayoutCoords->addWidget(lonSpinBox);


        verticalLayout->addWidget(groupBoxCoordinates);

        groupBoxLayers = new QGroupBox(centralwidget);
        groupBoxLayers->setObjectName("groupBoxLayers");
        horizontalLayoutLayers = new QHBoxLayout(groupBoxLayers);
        horizontalLayoutLayers->setObjectName("horizontalLayoutLayers");
        chkBuildings = new QCheckBox(groupBoxLayers);
        chkBuildings->setObjectName("chkBuildings");
        chkBuildings->setChecked(false);

        horizontalLayoutLayers->addWidget(chkBuildings);

        chkGreenZones = new QCheckBox(groupBoxLayers);
        chkGreenZones->setObjectName("chkGreenZones");
        chkGreenZones->setChecked(true);

        horizontalLayoutLayers->addWidget(chkGreenZones);

        chkWater = new QCheckBox(groupBoxLayers);
        chkWater->setObjectName("chkWater");
        chkWater->setChecked(true);

        horizontalLayoutLayers->addWidget(chkWater);

        chkRailways = new QCheckBox(groupBoxLayers);
        chkRailways->setObjectName("chkRailways");
        chkRailways->setChecked(true);

        horizontalLayoutLayers->addWidget(chkRailways);


        verticalLayout->addWidget(groupBoxLayers);

        verticalSpacer = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);
        progressBar->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(progressBar);

        compileButton = new QPushButton(centralwidget);
        compileButton->setObjectName("compileButton");
        compileButton->setMinimumSize(QSize(0, 40));

        verticalLayout->addWidget(compileButton);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "ESP32 Bike Map Preprocessor", nullptr));
        labelInput->setText(QCoreApplication::translate("MainWindow", "Input OSM File:", nullptr));
        inputPathEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Select path to your .osm map extract file...", nullptr));
        browseInputButton->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
        labelOutput->setText(QCoreApplication::translate("MainWindow", "Output Binary:", nullptr));
        outputPathEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Select path to save packed .bin asset...", nullptr));
        browseOutputButton->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
        groupBoxCoordinates->setTitle(QCoreApplication::translate("MainWindow", "Map Center Reference Anchor (Metric Zero Origin)", nullptr));
        labelLat->setText(QCoreApplication::translate("MainWindow", "Latitude:", nullptr));
        labelLon->setText(QCoreApplication::translate("MainWindow", "Longitude:", nullptr));
        groupBoxLayers->setTitle(QCoreApplication::translate("MainWindow", "Map Layer Toggles (File Size Optimization)", nullptr));
        chkBuildings->setText(QCoreApplication::translate("MainWindow", "Buildings", nullptr));
        chkGreenZones->setText(QCoreApplication::translate("MainWindow", "Green Zones", nullptr));
        chkWater->setText(QCoreApplication::translate("MainWindow", "Water", nullptr));
        chkRailways->setText(QCoreApplication::translate("MainWindow", "Railways", nullptr));
        compileButton->setText(QCoreApplication::translate("MainWindow", "Compile Map for ESP32-S3", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
