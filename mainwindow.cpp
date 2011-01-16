#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>

#define DEFAULT_CAMERA_TIMER_MS 100   // 0.1-second timer

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initializing the default camera
    camera = cvCreateCameraCapture(0);
    assert(camera);
    IplImage *image=cvQueryFrame(camera);
    assert(image);

    printf("Image depth=%i\n", image->depth);
    printf("Image nChannels=%i\n", image->nChannels);

    ui->cameraWidget->setCascadeName(ui->cascadesCB->currentText());

    ui->cameraWidget->setHsvHueInterval(ui->hsvHueSB->value());
    ui->cameraWidget->setHsvBrightnessThreshold(ui->hsvBrightnessThresholdSB->value());
    ui->cameraWidget->setHsvSaturationThreshold(ui->hsvSaturationThresholdSB->value());

    cameraTimerId=startTimer(DEFAULT_CAMERA_TIMER_MS);
}

MainWindow::~MainWindow()
{
    cvReleaseCapture(&camera);

    delete ui;
}


void MainWindow::timerEvent(QTimerEvent*) {
    IplImage *image=cvQueryFrame(camera);
    ui->cameraWidget->putImage(image);
    ui->objsFoundLabel->setText(QString::number(ui->cameraWidget->getObjsFound()));
    ui->calcTimeLabel->setText(QString::number(ui->cameraWidget->getODCalcTime()).append(" ms"));
    ui->imageSizeInfoLabel->setText(QString("Image size: ").append(QString::number(image->width)).append("x").append(QString::number(image->height)));
}


void MainWindow::on_faceDetectionEnabled_clicked(bool checked)
{

}

void MainWindow::on_pushButton_clicked()
{
    killTimer(cameraTimerId);
    cameraTimerId=startTimer(ui->timeOutCamera->value());
}
