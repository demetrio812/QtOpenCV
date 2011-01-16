#include "qopencvimagebox.h"
#include "ui_qopencvimagebox.h"

#define DEFAULT_IMAGE_SIZE 8
#define DEFAULT_CASCADES_PATH "/opt/local/share/opencv/haarcascades/"

QOpenCvImageBox::QOpenCvImageBox(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QOpenCvImageBox)
{
    ui->setupUi(this);

    QImage dummy(1,1,QImage::Format_RGB32);
    image = dummy;
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));

    detectionEnabled=false;
    hsvAllImageConvertedEnabled=false;
}

QOpenCvImageBox::~QOpenCvImageBox()
{
    delete ui;
}

void QOpenCvImageBox::putImage(IplImage *cvimage) {
    if (isHsvAllImageConvertedEnabled()) {
        // HSV conversion
        //hsvReduce(cvimage);
        cvimage=GlViewColor(cvimage);
    }

    // Face detection
    if (isDetectionEnabled()) {  // If I have a cascade file
        if (detected_objs_array!=NULL) {  // release the memory
            delete [] detected_objs_array;
        }
        detected_objs_array=detect_objs(cvimage, storage, cascade);
        draw_objs(cvimage);
    }

    //old_detect_and_draw_faces(cvimage, storage, cascade);

    // Image conversion
    int cvIndex, cvLineStart;
    // switch between bit depths
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
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
}

// HSV

bool QOpenCvImageBox::isHsvAllImageConvertedEnabled() {
    return hsvAllImageConvertedEnabled;
}

void QOpenCvImageBox::setHsvAllImageConvertedEnabled(bool value) {
    hsvAllImageConvertedEnabled=value;
}

#define HUE_INTERVAL 42

#define CORREZIONE_LUMINOSITA 50
#define CORREZIONE_SATURAZIONE 20

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

        if (value<CORREZIONE_LUMINOSITA) { // Se ci si avvicina al nero lo metto nero
            hue=255;
            saturation=255;
            value=0;
        } else if (saturation<CORREZIONE_SATURAZIONE) { // se la saturazione Ë vicina allo zero la setto a zero(bianco)
            hue=0;
            saturation=0;
            value=255;
        } else {
            saturation=255;
            value=255;

            if ((hue>=0 && hue<=50) || (hue>210 && hue<=255)){
                if (hue>28) {
                    hue=40;
                } else {
                    hue=0;
                }
            } else if (hue>=128 && hue<=210) {
                hue=168;
            } else {
                hue=84;
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

CvRect* QOpenCvImageBox::detect_objs( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade )
{
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
                                            1.1, // scale_factor
                                            3, // min_neighbors
                                            0
                                            //|CV_HAAR_FIND_BIGGEST_OBJECT
                                            //|CV_HAAR_DO_ROUGH_SEARCH
                                            |CV_HAAR_DO_CANNY_PRUNING
                                            //|CV_HAAR_SCALE_IMAGE
                                            ,
                                            cvSize(0, 0) );
        t = (double)cvGetTickCount() - t;

        //printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
        od_calc_time=t/((double)cvGetTickFrequency()*1000.);

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

void QOpenCvImageBox::draw_objs( IplImage* img)
{
    // Create two points to represent the face locations
    CvPoint pt1, pt2;

    for(int i = 0; i < objs_found; i++ )
    {

        CvRect* r = &detected_objs_array[i];

        if(r) {
            // Find the dimensions of the face,and scale it if necessary
            pt1.x = r->x*od_image_scale;
            pt2.x = (r->x+r->width)*od_image_scale;
            pt1.y = r->y*od_image_scale;
            pt2.y = (r->y+r->height)*od_image_scale;

            // Draw the rectangle in the input image
            cvRectangle( img, pt1, pt2, CV_RGB(255,0,0), 1, 8, 0 );
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
