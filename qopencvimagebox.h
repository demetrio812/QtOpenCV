#ifndef QOPENCVIMAGEBOX_H
#define QOPENCVIMAGEBOX_H

#include <QWidget>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/video/tracking.hpp>

#include <iostream>
#include <ctype.h>

using namespace cv;
using namespace std;

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

    bool isSubDetectionEnabled();
    int getSubObjsFound();
    double getSubODCalcTime();

public slots:
    void setHsvAllImageConvertedEnabled(bool value);
    void setHsvHueInterval(int value);
    void setHsvBrightnessThreshold(int value);
    void setHsvSaturationThreshold(int value);

    void setDetectionEnabled(bool value);
    void setODImageScale(int value);
    void setCascadeName(QString cascade_path);

    void setSubDetectionEnabled(bool value);
    void setSubODImageScale(int value);
    void setSubObjsCascadeName(QString cascade_path);

private:
    Ui::QOpenCvImageBox *ui;

    // Image conversion
    QImage convertToQImage(IplImage *cvimage);

    // Enabled/Disable features
    bool hsvAllImageConvertedEnabled;
    bool detectionEnabled;
    bool subDetectionEnabled;

    // HSV
    void hsvReduce(IplImage *cvimage);
    int hsv_hue_interval;
    int hsv_brightness_threshold;
    int hsv_saturation_threshold;

    // Kinect
    IplImage *GlViewColor(IplImage *depth);

    // Objs detection
    int objs_found; // number of visible faces
    int visible; // if someone is visible
    CvRect *detected_objs_array;
    int od_calc_time;
    int od_image_scale;
    QString *cascade_name;

    CvHaarClassifierCascade* cascade;
    CvMemStorage* storage;

    // Sub objs detection
    int sub_objs_found; // number of visible faces
    int sub_objs_visible; // if someone is visible
    CvRect *detected_sub_objs_array;
    int sub_od_calc_time;
    int sub_od_image_scale;
    QString *sub_obj_cascade_name;

    CvHaarClassifierCascade* sub_cascade;
    CvMemStorage* sub_storage;

    // Generic obj detection
    void old_detect_and_draw_faces( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade );

    CvRect* detect_objs( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade, int image_scale, int &objs_found, int &calc_time, double scale_factor = 1.1, int min_neighbors = 3 );
    void detectImage(IplImage* picture);
    void draw_objs(IplImage* img, int objs_found, CvRect *objs_array, CvScalar color, int image_scale, int x_offset, int y_offset);

    // Lucas-Kanade optical flow
    int MAX_COUNT;
    bool needToInit;
    bool nightMode;
    Mat gray, prevGray, image;
    vector<Point2f> points[2];

    void lucasKanadeOF( IplImage* img );
};

#endif // QOPENCVIMAGEBOX_H
