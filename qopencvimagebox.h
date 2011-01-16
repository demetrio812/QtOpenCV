#ifndef QOPENCVIMAGEBOX_H
#define QOPENCVIMAGEBOX_H

#include <QWidget>
#include <opencv/cv.h>
#include <opencv/highgui.h>

namespace Ui {
    class QOpenCvImageBox;
}

class QOpenCvImageBox : public QWidget
{
    Q_OBJECT

public:
    explicit QOpenCvImageBox(QWidget *parent = 0);
    ~QOpenCvImageBox();

    void putImage(IplImage *);
    bool isHsvAllImageConvertedEnabled();
    bool isDetectionEnabled();

    int getObjsFound();
    double getODCalcTime();

public slots:

    void setHsvAllImageConvertedEnabled(bool value);
    void setHsvHueInterval(int value);
    void setHsvBrightnessThreshold(int value);
    void setHsvSaturationThreshold(int value);

    void setDetectionEnabled(bool value);

    void setODImageScale(int value);
    void setCascadeName(QString cascade_path);

private:
    Ui::QOpenCvImageBox *ui;

    QImage image;

    // Enabled/Disable features
    bool hsvAllImageConvertedEnabled;
    bool detectionEnabled;

    // HSV
    void hsvReduce(IplImage *cvimage);
    int hsv_hue_interval;
    int hsv_brightness_threshold;
    int hsv_saturation_threshold;

    // Kinect
    IplImage *GlViewColor(IplImage *depth);

    // Face detection

    int objs_found; // number of visible faces
    int visible; // if someone is visible
    CvRect *detected_objs_array;

    int od_calc_time;

    int od_image_scale;  // default=2

    CvHaarClassifierCascade* cascade;
    CvMemStorage* storage;

    QString *cascade_name;

    void old_detect_and_draw_faces( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade );

    CvRect* detect_objs( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade );
    void detectImage(IplImage* picture);
    void draw_objs(IplImage* img);

};

#endif // QOPENCVIMAGEBOX_H
