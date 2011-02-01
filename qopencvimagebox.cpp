#include "qopencvimagebox.h"
#include "ui_qopencvimagebox.h"

#define DEFAULT_IMAGE_SIZE 8
#define DEFAULT_SUB_IMAGE_SIZE 1
#define DEFAULT_CASCADES_PATH "/opt/local/share/opencv/haarcascades/"

QOpenCvImageBox::QOpenCvImageBox(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::QOpenCvImageBox)
{
    ui->setupUi(this);

    QImage dummy(1,1,QImage::Format_RGB32);
    ui->imageLabel->setPixmap(QPixmap::fromImage(dummy));

    detectionEnabled=false;
    hsvAllImageConvertedEnabled=false;
    subDetectionEnabled=false;

    setHsvHueInterval(42);
    setHsvBrightnessThreshold(50);
    setHsvSaturationThreshold(20);

    // Lucas-Kanade optical flow
    MAX_COUNT = 500;
    needToInit = false;
    nightMode = false;
}

QOpenCvImageBox::~QOpenCvImageBox()
{
    delete ui;
}

void QOpenCvImageBox::putImage(IplImage *cvimage) {
    if (isHsvAllImageConvertedEnabled()) {
        // HSV conversion
        hsvReduce(cvimage);
    }

    // Objs detection
    if (isDetectionEnabled()) {  // If I have a cascade file
        if (detected_objs_array!=NULL) {  // release the memory
            delete [] detected_objs_array;
        }
        detected_objs_array=detect_objs(cvimage, storage, cascade, od_image_scale, objs_found, od_calc_time);
        draw_objs(cvimage, objs_found, detected_objs_array, CV_RGB(255,0,0), od_image_scale, 0, 0);

        // Sub Objs detection
        if (isSubDetectionEnabled()) {  // If I have a cascade file
            for (int c=0; c<objs_found; c++) {
                // Finding the image part
                IplImage *cvSubImage=cvCreateImage(cvSize(detected_objs_array[0].width*od_image_scale, detected_objs_array[0].height*od_image_scale), IPL_DEPTH_8U, 3);

                // Adapt the rect to the scale
                CvRect scaledRect = cvRect(detected_objs_array[0].x*od_image_scale, detected_objs_array[0].y*od_image_scale, detected_objs_array[0].width*od_image_scale, detected_objs_array[0].height*od_image_scale);

                // Setting the ROI
                cvSetImageROI(cvimage, scaledRect);

                // Copying a sub part
                cvCopyImage(cvimage, cvSubImage);

                // Resetting the ROI
                cvResetImageROI(cvimage);

                if (detected_sub_objs_array!=NULL) {  // release the memory
                    delete [] detected_sub_objs_array;
                }
                detected_sub_objs_array=detect_objs(cvSubImage, sub_storage, sub_cascade, sub_od_image_scale, sub_objs_found, sub_od_calc_time);

                ui->imageLabel->setPixmap(QPixmap::fromImage(convertToQImage(cvSubImage)));

                draw_objs(cvimage, objs_found, detected_sub_objs_array, CV_RGB(0,255,0), sub_od_image_scale, scaledRect.x, scaledRect.y);
            }
        }
    }

    lucasKanadeOF(cvimage);

    // Image conversion
    ui->imageLabel->setPixmap(QPixmap::fromImage(convertToQImage(cvimage)));
}

// Image conversion
QImage QOpenCvImageBox::convertToQImage(IplImage *cvimage) {
    QImage image;

    int cvIndex, cvLineStart;
    // switch between bit depths
    // TODO other cases
    switch (cvimage->depth) {
    case IPL_DEPTH_8U:
        switch (cvimage->nChannels) {
        case 3:
            if ( (cvimage->width != image.width()) || (cvimage->height != image.height()) ) {
                QImage temp(cvimage->width, cvimage->height, QImage::Format_RGB32);
                image = temp;
            }
            cvIndex = 0; cvLineStart = 0;
            for (int y = 0; y < cvimage->height; y++) {
                unsigned char red,green,blue;
                cvIndex = cvLineStart;
                for (int x = 0; x < cvimage->width; x++) {
                    // DO it
                    red = cvimage->imageData[cvIndex+2];
                    green = cvimage->imageData[cvIndex+1];
                    blue = cvimage->imageData[cvIndex+0];

                    image.setPixel(x,y,qRgb(red, green, blue));
                    cvIndex += 3;
                }
                cvLineStart += cvimage->widthStep;
            }
            break;
                default:
            printf("This number of channels is not supported\n");
            break;
        }
        break;
        default:
        printf("This type of IplImage is not implemented in QOpenCVWidget\n");
        break;
    }

    return image;
}

// HSV

bool QOpenCvImageBox::isHsvAllImageConvertedEnabled() {
    return hsvAllImageConvertedEnabled;
}

void QOpenCvImageBox::setHsvAllImageConvertedEnabled(bool value) {
    hsvAllImageConvertedEnabled=value;
}

void QOpenCvImageBox::setHsvHueInterval(int value) {
    hsv_hue_interval=value;
}

void QOpenCvImageBox::setHsvBrightnessThreshold(int value) {
    hsv_brightness_threshold=value;
}

void QOpenCvImageBox::setHsvSaturationThreshold(int value) {
    hsv_saturation_threshold=value;
}

void QOpenCvImageBox::hsvReduce(IplImage *cvimage) {
    IplImage *hsvImage = cvCreateImage( cvSize(cvimage->width,cvimage->height), IPL_DEPTH_8U, 3 );
    cvCvtColor(cvimage,hsvImage,CV_BGR2HSV);

    int heightc = hsvImage->height;
    int widthc = hsvImage->width;
    int stepc=hsvImage->widthStep;
    int channelsc=hsvImage->nChannels;

    uchar *datac = (uchar *)hsvImage->imageData;

    for(int i=0;i< (heightc);i++) for(int j=0;j<(widthc);j++) {
        int saturation=datac[i*stepc+j*channelsc+1];
        int value=datac[i*stepc+j*channelsc+2];

        // normalizing the hue
        int hue=datac[i*stepc+j*channelsc];

        //int intervalFound=hue/HUE_INTERVAL;  // es. 100/6 = 6.25

        // round the intervalFound
        //round(intervalFound);

        // find the normalized hue
        //int hueFound=intervalFound*HUE_INTERVAL;

        //printf("hueFound: %d", hueFound);

        if (value<hsv_brightness_threshold) { // Se ci si avvicina al nero lo metto nero
            hue=255;
            saturation=255;
            value=0;
        } else if (saturation<hsv_saturation_threshold) { // se la saturazione Ë vicina allo zero la setto a zero(bianco)
            hue=0;
            saturation=0;
            value=255;
        } else {
            saturation=255;
            value=255;

            // reducing the hue values (so the colors)
            double intervalFound=hue/hsv_hue_interval;

            if (intervalFound==0) {
                hue=0;
            } else {
                // round the intervalFound
                round(intervalFound);

                int hueFound=255/intervalFound;  // finding the color in the rounded value

                hue=hueFound;
            }
        }

        // copy into the value
        datac[i*stepc+j*channelsc]=hue;
        datac[i*stepc+j*channelsc+1]=saturation;
        datac[i*stepc+j*channelsc+2]=value;
    }

    cvCvtColor(hsvImage,cvimage,CV_HSV2BGR);
    cvReleaseImage(&hsvImage);
}

IplImage *QOpenCvImageBox::GlViewColor(IplImage *depth)
{
    static IplImage *image = 0;
    if (!image) image = cvCreateImage(cvSize(640,480), 8, 3);
    unsigned char *depth_mid = (unsigned char *)image->imageData;
    int i;
    for (i = 0; i < 640*480; i++) {
        int lb = ((short *)depth->imageData)[i] % 256;
        int ub = ((short *)depth->imageData)[i] / 256;
        switch (ub) {
        case 0:
            depth_mid[3*i+2] = 255;
            depth_mid[3*i+1] = 255-lb;
            depth_mid[3*i+0] = 255-lb;
            break;
        case 1:
            depth_mid[3*i+2] = 255;
            depth_mid[3*i+1] = lb;
            depth_mid[3*i+0] = 0;
            break;
        case 2:
            depth_mid[3*i+2] = 255-lb;
            depth_mid[3*i+1] = 255;
            depth_mid[3*i+0] = 0;
            break;
        case 3:
            depth_mid[3*i+2] = 0;
            depth_mid[3*i+1] = 255;
            depth_mid[3*i+0] = lb;
            break;
        case 4:
            depth_mid[3*i+2] = 0;
            depth_mid[3*i+1] = 255-lb;
            depth_mid[3*i+0] = 255;
            break;
        case 5:
            depth_mid[3*i+2] = 0;
            depth_mid[3*i+1] = 0;
            depth_mid[3*i+0] = 255-lb;
            break;
        default:
            depth_mid[3*i+2] = 0;
            depth_mid[3*i+1] = 0;
            depth_mid[3*i+0] = 0;
            break;
        }
    }
    return image;
}

// Obj detection

bool QOpenCvImageBox::isDetectionEnabled() {
    return detectionEnabled;
}

void QOpenCvImageBox::setDetectionEnabled(bool value) {
    detectionEnabled=value;
    if (detectionEnabled) {
        // FD initialization
        detected_objs_array=NULL;
        objs_found=0;
        visible=0;
        od_calc_time=0;

        od_image_scale=DEFAULT_IMAGE_SIZE;

        cascade = (CvHaarClassifierCascade*)cvLoad(cascade_name->toLatin1(), 0, 0, 0 );
        if(!cascade)
        {
            printf("Could not open cascade classifier %s", cascade_name);
        }
    } else {
        cvReleaseHaarClassifierCascade(&cascade);
    }

    // Enable/Disable the sub objs detection

}

int QOpenCvImageBox::getObjsFound() {
    if (isDetectionEnabled()) {
        return objs_found;
    } else {
        return 0;
    }
}

void QOpenCvImageBox::setODImageScale(int value) {
    od_image_scale=value;
}

double QOpenCvImageBox::getODCalcTime() {
    return od_calc_time;
}

void QOpenCvImageBox::setCascadeName(QString cascade_path) {
    this->cascade_name=new QString(DEFAULT_CASCADES_PATH);
    this->cascade_name->append(cascade_path);

    // If the detection is enabled: reinitialise it
    if (isDetectionEnabled()) {
        setDetectionEnabled(false);
        setDetectionEnabled(true);
    }
}

// Sub Obj detection

bool QOpenCvImageBox::isSubDetectionEnabled() {
    return subDetectionEnabled;
}

void QOpenCvImageBox::setSubDetectionEnabled(bool value) {
    subDetectionEnabled=value;
    if (subDetectionEnabled) {
        // FD initialization
        detected_sub_objs_array=NULL;
        sub_objs_found=0;
        sub_objs_visible=0;
        sub_od_calc_time=0;

        sub_od_image_scale=DEFAULT_SUB_IMAGE_SIZE;

        sub_cascade = (CvHaarClassifierCascade*)cvLoad(sub_obj_cascade_name->toLatin1(), 0, 0, 0 );
        if(!sub_cascade)
        {
            printf("Could not open cascade classifier %s", cascade_name);
        }
    } else {
        cvReleaseHaarClassifierCascade(&sub_cascade);
    }
}

int QOpenCvImageBox::getSubObjsFound() {
    if (isSubDetectionEnabled()) {
        return sub_objs_found;
    } else {
        return 0;
    }
}

void QOpenCvImageBox::setSubODImageScale(int value) {
    sub_od_image_scale=value;
}

double QOpenCvImageBox::getSubODCalcTime() {
    return sub_od_calc_time;
}

void QOpenCvImageBox::setSubObjsCascadeName(QString cascade_path) {
    this->sub_obj_cascade_name=new QString(DEFAULT_CASCADES_PATH);
    this->sub_obj_cascade_name->append(cascade_path);

    // If the detection is enabled: reinitialise it
    if (isSubDetectionEnabled()) {
        setSubDetectionEnabled(false);
        setSubDetectionEnabled(true);
    }
}

// Generic detection

CvRect* QOpenCvImageBox::detect_objs( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade, int image_scale, int &objs_found, int &calc_time, double scale_factor, int min_neighbors )
{
    IplImage *gray, *small_img;
    int i;

    if( cascade )
    {
        storage = cvCreateMemStorage(0);

        gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
        small_img = cvCreateImage( cvSize( cvRound (img->width/image_scale), cvRound (img->height/image_scale)), 8, 1 );

        cvCvtColor( img, gray, CV_RGB2GRAY );
        cvResize( gray, small_img, CV_INTER_LINEAR );
        cvEqualizeHist( small_img, small_img );
        cvClearMemStorage( storage );

        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            scale_factor,
                                            min_neighbors,
                                            0
                                            //|CV_HAAR_FIND_BIGGEST_OBJECT
                                            //|CV_HAAR_DO_ROUGH_SEARCH
                                            |CV_HAAR_DO_CANNY_PRUNING
                                            //|CV_HAAR_SCALE_IMAGE
                                            ,
                                            cvSize(0, 0) );
        t = (double)cvGetTickCount() - t;

        //printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
        calc_time=t/((double)cvGetTickFrequency()*1000.);

        // Update the number
        objs_found = faces->total;

        cvReleaseImage( &gray );
        cvReleaseImage( &small_img );

        // Loop the number of faces found.
        //printf("Detected %d faces!\n", faces->total);

        CvRect* farray=new CvRect[objs_found];
        for( i = 0; i < (faces ? objs_found : 0); i++ )
        {
            // Create a new rectangle for drawing the face
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );

            farray[i].x=r->x;
            farray[i].y=r->y;
            farray[i].width=r->width;
            farray[i].height=r->height;
        }

        cvClearSeq(faces);
        cvReleaseMemStorage(&storage);

        return farray;
    }

    return NULL;
}

void QOpenCvImageBox::draw_objs( IplImage* img, int objs_found, CvRect *objs_array, CvScalar color, int image_scale, int x_offset, int y_offset)
{
    // Create two points to represent the face locations
    CvPoint pt1, pt2;

    for(int i = 0; i < objs_found; i++ )
    {

        CvRect* r = &objs_array[i];

        if(r) {
            // Find the dimensions of the face,and scale it if necessary
            pt1.x = x_offset+r->x*image_scale;
            pt2.x = x_offset+(r->x+r->width)*image_scale;
            pt1.y = y_offset+r->y*image_scale;
            pt2.y = y_offset+(r->y+r->height)*image_scale;

            // Draw the rectangle in the input image
            cvRectangle( img, pt1, pt2, color, 1, 8, 0 );
        }
    }
}

void QOpenCvImageBox::old_detect_and_draw_faces( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade )
{
    // Create two points to represent the face locations
    CvPoint pt1, pt2;

    IplImage *gray, *small_img;
    int i;

    if( cascade )
    {
        storage = cvCreateMemStorage(0);

        gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
        small_img = cvCreateImage( cvSize( cvRound (img->width/od_image_scale), cvRound (img->height/od_image_scale)), 8, 1 );

        cvCvtColor( img, gray, CV_RGB2GRAY );
        cvResize( gray, small_img, CV_INTER_LINEAR );
        cvEqualizeHist( small_img, small_img );
        cvClearMemStorage( storage );

        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            1.1, 2, 0
                                            //|CV_HAAR_FIND_BIGGEST_OBJECT
                                            //|CV_HAAR_DO_ROUGH_SEARCH
                                            |CV_HAAR_DO_CANNY_PRUNING
                                            //|CV_HAAR_SCALE_IMAGE
                                            ,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;

        printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

        cvReleaseImage( &gray );
        cvReleaseImage( &small_img );

        // Loop the number of faces found.
        printf("Detected %d faces!\n", faces->total);

        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            // Create a new rectangle for drawing the face
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );

            if(r) {
                // Find the dimensions of the face,and scale it if necessary
                pt1.x = r->x*od_image_scale;
                pt2.x = (r->x+r->width)*od_image_scale;
                pt1.y = r->y*od_image_scale;
                pt2.y = (r->y+r->height)*od_image_scale;

                // Draw the rectangle in the input image
                cvRectangle( img, pt1, pt2, CV_RGB(255,0,0), 3, 8, 0 );
            }
        }

        cvClearSeq(faces);
        cvReleaseMemStorage(&storage);
    }
}

// Lucas-Kanade optical flow
void QOpenCvImageBox::lucasKanadeOF( IplImage* img ) {
    Point2f pt;
    TermCriteria termcrit(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03);  // ERROR

    bool addRemovePt = false;

    Size winSize(10,10);

    cvtColor(image, gray, CV_BGR2GRAY);

    if( nightMode )
        image = Scalar::all(0);

    if( needToInit )
    {
        // automatic initialization
        goodFeaturesToTrack(gray, points[1], MAX_COUNT, 0.01, 10, Mat(), 3, 0, 0.04);
        cornerSubPix(gray, points[1], winSize, Size(-1,-1), termcrit);
        addRemovePt = false;
    }
    else if( !points[0].empty() )
    {
        vector<uchar> status;
        vector<float> err;
        if(prevGray.empty())
            gray.copyTo(prevGray);
        calcOpticalFlowPyrLK(prevGray, gray, points[0], points[1], status, err, winSize, 3, termcrit, 0);
        size_t i, k;
        for( i = k = 0; i < points[1].size(); i++ )
        {
            if( addRemovePt )
            {
                if( norm(pt - points[1][i]) <= 5 )
                {
                    addRemovePt = false;
                    continue;
                }
            }

            if( !status[i] )
                continue;

            points[1][k++] = points[1][i];
            circle( image, points[1][i], 3, Scalar(0,255,0), -1, 8);
        }
        points[1].resize(k);
    }

    if( addRemovePt && points[1].size() < (size_t)MAX_COUNT )
    {
        vector<Point2f> tmp;
        tmp.push_back(pt);
        cornerSubPix( gray, tmp, winSize, cvSize(-1,-1), termcrit);
        points[1].push_back(tmp[0]);
        addRemovePt = false;
    }

    needToInit = false;
    /*
    imshow("LK Demo", image);

    char c = (char)waitKey(10);
    if( c == 27 )
        break;
    switch( c )
    {
    case 'r':
        needToInit = true;
        break;
    case 'c':
        points[1].clear();
        break;
    case 'n':
        nightMode = !nightMode;
        break;
    default:
        ;
    }
    */

    std::swap(points[1], points[0]);
    swap(prevGray, gray);
}
