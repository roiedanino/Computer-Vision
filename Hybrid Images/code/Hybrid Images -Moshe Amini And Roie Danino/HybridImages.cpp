//
// Created by Moshe Amini and Roie Danino on 18/12/2017.
//

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/opencv.hpp>

#include <iostream>

using namespace cv;
using namespace std;

void createPyramid(const Mat &src, vector<Mat> &gaussians, vector<Mat> &laplacians, int levels);

Mat hybridImages(const vector<Mat> &Gaussean1, const vector<Mat> &laplasian2, int level);

string interactiveConsoleInput(Mat& src1, Mat& src2);

bool listeners(const string& trackbar, const string& trackbar2, const string& windowName,int& n, int& m, bool& swapImages, bool& withColor);

int main()
{
	const int MAX_TRACK = 7, SIZE = 512;
	const char* hybridName = "Hybrid Image", *hybridTrackbarName = "Hybrid Level", *zoomTrackbarName = "Zoom Level";

    bool swapImages = false, withColor = true, hasChanged = true;
    //n: hybrid level, m: zoom level
    int pyr_Level = 0, n = 0, prev_n = n, m = 1, prev_m = m, zoom = 0;

    vector<Mat> src1Gaussians, src1Laplacians, src2Gaussians, src2Laplacians;
	Mat src1, src2, hybridImage, greyHybrid;

    string path = interactiveConsoleInput(src1, src2);

    //resize both images to the same size for compatibility
    resize(src1, src1, Size(SIZE, SIZE));
    resize(src2, src2, Size(src1.cols, src1.rows));

	namedWindow(hybridName, WINDOW_GUI_NORMAL);

	src1.convertTo(src1, CV_64FC3);
	src2.convertTo(src2,CV_64FC3);

	//Generating Gaussian and Laplacian pyramids for each of the images
    createPyramid(src1, src1Gaussians, src1Laplacians, MAX_TRACK);
    createPyramid(src2, src2Gaussians, src2Laplacians, MAX_TRACK);

    //a track bar for the Gaussians / Laplacians level
	createTrackbar(hybridTrackbarName, hybridName, &pyr_Level, MAX_TRACK);

    //a track bar for zoom
	createTrackbar(zoomTrackbarName, hybridName, &zoom, MAX_TRACK * 3);

    for (;;)
	{
        hasChanged = listeners(hybridTrackbarName, zoomTrackbarName, hybridName, n,m, swapImages, withColor);

		if(prev_n != n || prev_m != m || hasChanged)
		{

            if(!swapImages)
                hybridImage = hybridImages(src1Gaussians, src2Laplacians, n);

            else
                hybridImage = hybridImages(src2Gaussians, src1Laplacians, n);

            if(withColor)
                hybridImage.convertTo(hybridImage, CV_8UC1);

            else
            {
                hybridImage.convertTo(hybridImage, CV_8UC1);
                cvtColor(hybridImage, greyHybrid, CV_BGR2GRAY );
                hybridImage = greyHybrid;
            }

            prev_n = n;
            prev_m = m;

            m += 1;

            resize(hybridImage, hybridImage, Size(hybridImage.cols/m, hybridImage.rows/m));

            imshow(hybridName, hybridImage);
        }
	}
    return 0;
}

void createPyramid(const Mat &src, vector<Mat> &gaussians, vector<Mat> &laplacians, int levels)
{
    Mat temp = src.clone(), temp2;

    gaussians.push_back( src.clone());

    for (int i = 1; i < levels + 1; i++)
    {

        pyrDown(temp, temp, Size(temp.cols / 2, temp.rows / 2));

        gaussians.push_back(temp.clone());

        Mat prevGaussian = gaussians.at((unsigned long)i - 1), currentLaplacian;

        resize(temp, temp2,Size(prevGaussian.cols, prevGaussian.rows));
        subtract(prevGaussian,temp2,currentLaplacian);

        laplacians.push_back(currentLaplacian.clone());
    }
}

Mat hybridImages(const vector<Mat> &Gaussean1, const vector<Mat> &laplasian2, int level)
{
    Mat temp;

    temp = Gaussean1[level].clone();

    for (int i = level - 1; i >= 0; i--)
    {
        resize(temp, temp, Size(laplasian2[i].cols, laplasian2[i].rows));
        add(temp, laplasian2[i], temp);
    }

    return temp;
}



string interactiveConsoleInput(Mat& src1, Mat& src2)
{
    bool validInput;
    string path, menu = " * [s] -> Swap Sources \n * [c] -> Active / Disable Color \n * [ESC] -> Close program \n \n";
    do {
        try {
            cout << "Enter first image full path (will be the low-passed image until swap):" << endl;
            cin >> path;
            src1 = imread(path);
            if(src1.empty())
                throw cv::Exception();

            cout << "Enter second image full (will be the high-passed image until swap):" << endl;
            cin >> path;
            src2 = imread(path);
            if(src2.empty())
                throw cv::Exception();

            validInput = true;
        }
        catch (cv::Exception &exception1) {
            cout  << endl << path << " is not a valid path! try again" << endl << endl;
        }
    }while(!validInput);

    cout << menu;
    return path;
}

bool listeners(const string& trackbar, const string& trackbar2, const string& windowName,int& n, int& m, bool& swapImages, bool& withColor)
{
    int c;
    bool hasChanged = false;
    n = getTrackbarPos(trackbar, windowName);
    m = getTrackbarPos(trackbar2, windowName);

    c = waitKey(10);

    switch(c)
    {
        case 's': case 'S':
            swapImages = !swapImages;
            hasChanged = true;
            break;

        case 'c': case 'C':
            withColor = !withColor;
            hasChanged = true;
            break;

        case 27:
            exit(0);

        default:
            break;
    }
    return hasChanged;
}
