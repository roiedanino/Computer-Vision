// OpenCV_Helloworld.cpp : Defines the entry point for the console application.
// Created for build/install tutorial, Microsoft Visual C++ 2010 Express and OpenCV 2.1.0


#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"
 #include "opencv2/video/background_segm.hpp"
#include <stdio.h>
#include <iostream>

using namespace cv;
using namespace std;

int main()
{
    bool CAMERA;
    int choice, sigma = 3, ksize, median = 9;
    double fps;
	VideoCapture cap; // video capture source

    string menu = "Background Subtraction from a Video and Live Camera Menu:\n1)Video\n2)Live Camera\n0) Exit\n";

    string VIDEO_PATH = "C:\\Users\\afeka\\Documents\\Visual Studio 2015\\Projects\\OPENCVtest3.3\\John Petrucci Under a Glass Moon Solo.mp4";

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
                break;
            case 2:
                CAMERA = true;
                break;

            default:
                cout << "Invalid option try again" << endl << endl;
        }

    } while(choice > 2 || choice < 0);

    if(CAMERA)
        cap.open(0);
    else
	    cap.open(VIDEO_PATH, WINDOW_GUI_EXPANDED);

	if (!cap.isOpened())
	{

		puts("***Could not initialize capturing...***\n");
		return 0;
	}

    namedWindow("Capture ", CV_WINDOW_FREERATIO);
	namedWindow("Foreground ", CV_WINDOW_FREERATIO);
    namedWindow("With Color",CV_WINDOW_FREERATIO);

	Mat frame, foreground, invertForground, image, res, merged, backBuffer, backDouble; // uses built in bkgrnd subtraction
	Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
	//create Background Subtractor objects
    pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach
	fps = cap.get(CV_CAP_PROP_FPS);

    if (fps <= 0)
		fps = 10;
	else
		fps =  1000 / fps;

    createTrackbar( "Gaussian Sigma", "With Color", &sigma, 15 );
    createTrackbar( "Median Sigma", "With Color", &median,15);

    for (;;)
	{
		cap >> frame;  // assign
        if (frame.empty())
			break;
		image = frame.clone(); //to avoid overwrite by the next frame

		res = image.clone(); //double buffered

		pMOG2->apply(res, foreground, -1);

		threshold(foreground, foreground, 20, 255, THRESH_TOZERO);

        ksize = (sigma*5)|1;

        median |= 1;

        GaussianBlur(foreground, foreground, Size(ksize, ksize), sigma, sigma);

        medianBlur(foreground, foreground, median);

        erode(foreground, foreground, Mat());
		dilate(foreground, foreground, Mat());

        threshold(merged, merged, 20, 255, THRESH_MASK);

        res.copyTo(merged,foreground);

        imshow("Capture ", image);

        imshow("Foreground ", foreground);

        imshow("With Color", merged);

        char c = (char)waitKey((int)fps);
		if (c == 27)   // ESC 
			break;

	}
}
