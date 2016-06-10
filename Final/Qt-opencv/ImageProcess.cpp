#include "ImageProcess.h"
#include "ui_subwindow.h"
#include <QLabel>
#include <QDebug>

ImageProcess::ImageProcess(QObject *parent) : QThread(parent) {
    subWidget = new QWidget();
    ui.setupUi(subWidget);


    method = 0;
    subWidget->show();
    xbee = new Xbee("/dev/ttyUSB0");
}

ImageProcess::~ImageProcess() {
    delete this;
}

void ImageProcess::run() {

    qDebug() << "New thread started successfully!!";

}

void ImageProcess::processImage(cv::Mat &img) {
    cv::Mat process;

    cv::Mat img_threshold;
    cv::Mat img_gray;
    cv::Mat img_roi;
    vector<vector<Point> > squares;
    char a[40];
    //int count =0;
    count = 0;

    Rect roi(340,100,270,270);
    img_roi=img(roi);
    cvtColor(img_roi,img_gray,CV_RGB2GRAY);

    GaussianBlur(img_gray,img_gray,Size(19,19),0.0,0);
    threshold(img_gray,img_threshold,0,255,THRESH_BINARY_INV+THRESH_OTSU);

    vector<vector<Point> >contours;
    vector<Vec4i>hierarchy;
    findContours(img_threshold,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,Point());
    if(contours.size()>0){
            size_t indexOfBiggestContour = -1;
            size_t sizeOfBiggestContour = 0;

            for (size_t i = 0; i < contours.size(); i++){
               if(contours[i].size() > sizeOfBiggestContour){
               sizeOfBiggestContour = contours[i].size();
               indexOfBiggestContour = i;
               }
            }
            vector<vector<int> >hull(contours.size());
            vector<vector<Point> >hullPoint(contours.size());
            vector<vector<Vec4i> >defects(contours.size());
            vector<vector<Point> >defectPoint(contours.size());
            vector<vector<Point> >contours_poly(contours.size());
            Point2f rect_point[4];
            vector<RotatedRect>minRect(contours.size());
            vector<Rect> boundRect(contours.size());
            for(size_t i=0;i<contours.size();i++){
                if(contourArea(contours[i])>5000){
                    convexHull(contours[i],hull[i],true);
                    convexityDefects(contours[i],hull[i],defects[i]);
                    if(indexOfBiggestContour==i){
                       minRect[i]=minAreaRect(contours[i]);
                       for(size_t k=0;k<hull[i].size();k++){
                            int ind=hull[i][k];
                            hullPoint[i].push_back(contours[i][ind]);
                       }
                        count =0;

                        for(size_t k=0;k<defects[i].size();k++){
                            if(defects[i][k][3]>13*256){
                                int p_end=defects[i][k][1];
                                int p_far=defects[i][k][2];
                                defectPoint[i].push_back(contours[i][p_far]);
                                circle(img_roi,contours[i][p_end],3,Scalar(0,255,0),2);
                                count++;
                            }

                        }

                        if(count==1)
                            strcpy(a,"Hello :D ");
                        else if(count==2)
                                    strcpy(a,"Peace :) ");
                        else if(count==3)
                                    strcpy(a,"3 it is !!");
                        else if(count==4)
                                    strcpy(a,"0100");
                        else if(count==5)
                            strcpy(a,"FIVE");
                        else
                            strcpy(a,"Welcome !!");

                        putText(img,a,Point(70,70),CV_FONT_HERSHEY_SIMPLEX,3,Scalar(255,0,0),2,8,false);
                        drawContours(img_threshold, contours, i,Scalar(255,255,0),2, 8, vector<Vec4i>(), 0, Point() );
                        drawContours(img_threshold, hullPoint, i, Scalar(255,255,0),1, 8, vector<Vec4i>(),0, Point());
                        drawContours(img_roi, hullPoint, i, Scalar(0,0,255),2, 8, vector<Vec4i>(),0, Point() );
                        approxPolyDP(contours[i],contours_poly[i],3,false);
                        boundRect[i]=boundingRect(contours_poly[i]);
                        rectangle(img_roi,boundRect[i].tl(),boundRect[i].br(),Scalar(255,0,0),2,8,0);
                        minRect[i].points(rect_point);
                        for(size_t k=0;k<4;k++){
                            line(img_roi,rect_point[k],rect_point[(k+1)%4],Scalar(0,255,0),2,8);
                        }

                    }
                }

            }
    }


    if(count == 3){
        xbee->setDir('r');
        xbee->start();
    }
    else{
        xbee->setDir('l');
        xbee->start();
    }


    // Determine how to process the image by mode
    /*
    switch(method) {
        case 0: process = image;
                break;
        case 1: process = edgeDetection(image);
                break;
        case 2: process = findCircles(image);
                break;
        case 3: findSquares(image, squares);
                drawSquares(image, squares);
                process = image;
                break;
        case 4: process = findLines(image);
                break;
        default:process = image;
                break;
    }
    */
    process = img;
    // Display processed image to another widget.
    QPixmap pix = QPixmap::fromImage(imageConvert(process));

    ui.displayLabel->setPixmap(pix);

    // Free the memory to avoid overflow.
    img.release();
}

cv::Mat ImageProcess::edgeDetection(cv::Mat &image) {
    cv::Mat edge;
    int lowThreshold = 10;
    int ratio = 3;
    int kernel_size = 3;

    // Convert it to gray
    cv::cvtColor(image, edge, CV_BGR2GRAY);

    // Run the edge detector on grayscale
    cv::blur(edge, edge, cv::Size(9,9));
    cv::Canny(edge, edge, lowThreshold, lowThreshold*ratio, kernel_size);

    return edge;
}

void ImageProcess::findSquares(cv::Mat &image, vector<vector<Point> > &squares) {
    squares.clear();

    Mat pyr, timg, gray0(image.size(), CV_8U), gray;
    int thresh = 50, N = 11;

    // down-scale and upscale the image to filter out the noise
    pyrDown(image, pyr, Size(image.cols/2, image.rows/2));
    pyrUp(pyr, timg, image.size());
    vector<vector<Point> > contours;

    // find squares in every color plane of the image
    for( int c = 0; c < 3; c++ )
    {
        int ch[] = {c, 0};
        cv::mixChannels(&timg, 1, &gray0, 1, ch, 1);

        // try several threshold levels
        for( int l = 0; l < N; l++ )
        {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging)
                cv::Canny(gray0, gray, 0, thresh, 5);
                // dilate canny output to remove potential
                // holes between edge segments
                cv::dilate(gray, gray, Mat(), Point(-1,-1));
            }
            else
            {
                // apply threshold if l!=0:
                // tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                gray = gray0 >= (l+1)*255/N;
            }

            // find contours and store them all as a list
            cv::findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

            vector<Point> approx;

            // test each contour
            for( size_t i = 0; i < contours.size(); i++ )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                cv::approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( approx.size() == 4 &&
                    fabs(contourArea(Mat(approx))) > 1000 &&
                    cv::isContourConvex(Mat(approx)) )
                {
                    double maxCosine = 0;

                    for( int j = 2; j < 5; j++ )
                    {
                        // find the maximum cosine of the angle between joint edges
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if( maxCosine < 0.3 )
                        squares.push_back(approx);
                }
            }
        }
    }
}

void ImageProcess::drawSquares(cv::Mat &image, vector<vector<Point> > &squares) {
    // the function draws all the squares in the image
    for( size_t i = 0; i < squares.size(); i++ ) {
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();

        polylines(image, &p, &n, 1, true, Scalar(0,255,0), 3, CV_AA);
    }
}

cv::Mat ImageProcess::findCircles(cv::Mat &image) {
    cv::Mat circle;
    vector<Vec3f> circles;

    // Convert it to gray
    cvtColor(image, circle, CV_BGR2GRAY);

    // Reduce the noise so we avoid false circle detection
    GaussianBlur(circle, circle, Size(9, 9), 2, 2);

    // Apply the Hough Transform to find the circles
    HoughCircles(circle, circles, CV_HOUGH_GRADIENT, 1, circle.rows/16, 150, 30, 0, 0);

    // Draw the circles detected
    for(size_t i = 0; i < circles.size(); i++ ) {
        cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);

        // circle center
        cv::circle(image, center, 3, cv::Scalar(0,255,0), -1, 8, 0 );
        // circle outline
        cv::circle(image, center, radius, cv::Scalar(0,0,255), 3, 8, 0 );
    }

    return image;
}

cv::Mat ImageProcess::findLines(cv::Mat &image) {
    cv::Mat line;
    vector<Vec4i> lines;

    // Convert it to gray
    cvtColor(image, line, CV_BGR2GRAY);

    // Detect the edges of the image by using a Canny detector
    Canny(line, line, 50, 200, 3);

    // Apply the Hough Transform to find the line segments
    HoughLinesP(line, lines, 1, CV_PI/180, 50, 50, 10);

    // Draw the line segments detected
    for(size_t i = 0; i < lines.size(); i++) {
      Vec4i l = lines[i];

      cv::line(image, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }

    return image;
}

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double ImageProcess::angle(Point pt1, Point pt2, Point pt0)
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

// The following slots are used to set image processing mode.
void ImageProcess::changeToNormal() {
    method = 0;
}

void ImageProcess::changeToEdgeDetection() {
    method = 1;
}

void ImageProcess::changeToCircleDetection() {
    method = 2;
}

void ImageProcess::changeToSquareDetection() {
    method = 3;
}

void ImageProcess::changeToLineDetection() {
    method = 4;
}

QImage ImageProcess::imageConvert(cv::Mat &matImage) {
    QImage::Format format;

    // If we use edge detection, we will use gray scale to display image.
    switch(method) {
        case 1: format = QImage::Format_Indexed8;
                break;
        default:format = QImage::Format_RGB888;
                break;
    }

    // Convert processed openCV frame to Qt's image format in order to display.
    QImage qImage(
        (uchar*)matImage.data,
        matImage.cols,
        matImage.rows,
        matImage.step,
        format
        );

    return qImage.rgbSwapped().mirrored(true, false);
}
