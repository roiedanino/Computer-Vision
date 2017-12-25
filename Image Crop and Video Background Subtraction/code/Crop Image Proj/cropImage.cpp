//
// Created by Roie Danino on 13/11/2017.
//

#include <iostream>
#include "opencv2/opencv.hpp"
#include <stdio.h>

using namespace std;
using namespace cv;

void checkBoundary();
void showImage();
void onMouse(int event, int x, int y, int f, void *);

VideoCapture cap;
Mat src, img, ROI, frame;
Rect cropRect(0, 0, 0, 0);
Point P1(0, 0);
Point P2(0, 0);

const char *winName = "Crop Image";
bool clicked = false;
int i = 0;
char imgName[15];



int main()
{
    bool CAMERA = true, VIDEO = true;
    int fps = 10, choice;
    string menu = "Cropping Images And Video Menu:\n1) Image\n2) Video\n3) Live Camera\n0) Exit\n";

    /*
     * Change this parameters to the full path of the files in your computer
     */

    string PHOTO_PATH = "C:\\Users\\afeka\\Documents\\Visual Studio 2015\\Projects\\OPENCVtest3.3 - Copy\\enhanced-31145-1409778141-5.jpg";

    string VIDEO_PATH = "C:\\Users\\afeka\\Documents\\Visual Studio 2015\\Projects\\OPENCVtest3.3 - Copy\\John Petrucci Under a Glass Moon Solo.mp4";

    do
    {
        cout << menu.c_str();
        cin >> choice;

        switch (choice)
        {
            case 0:
                return 0;
            case 1:
                CAMERA = false;
                VIDEO = false;
                break;
            case 2:
                CAMERA = false;
                VIDEO = true;
                break;
            case 3:
                CAMERA = true;
                VIDEO = true;
                break;
            default:
                cout << "Invalid option try again" << endl << endl;
        }
    } while(choice > 3 || choice < 0);

    cout << "Click and drag for Selection" << endl << endl;
    cout << "Press 'Esc' to quit" << endl << endl;


    if(CAMERA)
        cap.open(0);
    else
        cap.open(VIDEO_PATH); //change here to any video you would like to crop

    if (VIDEO)
    {
        cap >> frame;
        src = frame.clone();

        fps = cap.get(CV_CAP_PROP_FPS);

        if (fps <= 0)
            fps = 10;
        else
            fps = 1000 / fps;
    }

    else
    {
        src = imread(PHOTO_PATH, 1);
    }

    namedWindow(winName, WINDOW_NORMAL);
    setMouseCallback(winName, onMouse, NULL);

    imshow(winName, src);

    while (1)
    {

        if(VIDEO)
        {
            cap >> frame;

            src = frame.clone();

            if(!clicked)
                imshow(winName, src);
            else
                imshow(winName,img);

        }


        int c = waitKey(fps);


        if(clicked)
        {
            showImage();
        }
        if (c == 27)
            break;
    }
    return 0;
}

void checkBoundary()
{
    //check cropping rectangle exceed image boundary
    if (cropRect.width > img.cols - cropRect.x)
        cropRect.width = img.cols - cropRect.x;

    if (cropRect.height > img.rows - cropRect.y)
        cropRect.height = img.rows - cropRect.y;

    if (cropRect.x < 0)
        cropRect.x = 0;

    if (cropRect.y < 0)
        cropRect.height = 0;
}

void showImage()
{
    img = src.clone();
    checkBoundary();

    if (cropRect.width > 0 && cropRect.height > 0)
    {
        ROI = src(cropRect);
        imshow("cropped", ROI);
    }

    rectangle(img, cropRect, Scalar(255, 0, 0), 2, 8, 0);
    imshow(winName, img);

}


void onMouse(int event, int x, int y, int f, void *)
{

    switch (event)
    {
        case CV_EVENT_MOUSEMOVE    :
            if (clicked)
            {
                P2.x = x;
                P2.y = y;
            }
            break;

        case CV_EVENT_LBUTTONDOWN  :
            clicked = true;

            P1.x = x;
            P1.y = y;
            P2.x = x;
            P2.y = y;

            break;

        case CV_EVENT_LBUTTONUP    :
            P2.x = x;
            P2.y = y;
            clicked = false;

            break;

        default:
            break;
    }


    if (clicked)
    {
        if (P1.x > P2.x)
        {
            cropRect.x = P2.x;
            cropRect.width = P1.x - P2.x;

        }
        else
        {
            cropRect.x = P1.x;
            cropRect.width = P2.x - P1.x;
        }

        if (P1.y > P2.y)
        {
            cropRect.y = P2.y;
            cropRect.height = P1.y - P2.y;
        }
        else
        {
            cropRect.y = P1.y;
            cropRect.height = P2.y - P1.y;
        }
        showImage();
    }

}

