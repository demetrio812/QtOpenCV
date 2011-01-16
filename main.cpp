#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <assert.h>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include "mainwindow.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    MainWindow *fdWin = new MainWindow();
    fdWin->setWindowTitle("OpenCV and Qt");
    fdWin->show();

    int retval = app.exec();
    
    return retval;
}

