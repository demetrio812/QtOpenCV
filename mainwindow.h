#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv/cv.h>
#include <opencv/highgui.h>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    CvCapture *camera;

    int cameraTimerId;

protected:
    void timerEvent(QTimerEvent*);

private slots:
    void on_pushButton_clicked();
    void on_faceDetectionEnabled_clicked(bool checked);


};

#endif // MAINWINDOW_H
