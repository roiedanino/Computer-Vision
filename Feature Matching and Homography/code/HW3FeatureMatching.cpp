#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
using namespace std;
using namespace cv;

enum DetectionMethod {BRISK = 1, ORB = 2};

void interactiveConsoleInput(Mat& src1, Mat& src2, DetectionMethod& detectionMethod, double& threshold);


int main(int argc, char** argv)
{
    DetectionMethod detectionMethod;
    double threshold;
    Mat img_1, img_2;

    interactiveConsoleInput(img_1, img_2, detectionMethod, threshold);

	//-- Step 1: Detect the keypoints using BRISK or ORB Detector, compute the descriptors
    Ptr<FeatureDetector> detector;
    switch(detectionMethod)
    {
        case DetectionMethod::BRISK:
            detector = BRISK::create();
            break;
        case DetectionMethod::ORB:
            detector = ORB::create();
            break;
    }


	std::vector<KeyPoint> keypoints_1, keypoints_2;
	Mat descriptors_1, descriptors_2;
	detector->detectAndCompute(img_1, Mat(), keypoints_1, descriptors_1);
	detector->detectAndCompute(img_2, Mat(), keypoints_2, descriptors_2);

	//-- Step 2: Matching descriptor vectors using brute force descriptor matcher
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
	std::vector< DMatch > matches;
	matcher->match(descriptors_1, descriptors_2, matches, noArray());

	double max_dist = 0; double min_dist = threshold;//boxes - 225, buildings - 100
	//-- Quick calculation of max and min distances between keypoints
	for (int i = 0; i < descriptors_1.rows; i++)
	{
		double dist = matches[i].distance;
		if (dist < min_dist) min_dist = dist;
		if (dist > max_dist) max_dist = dist;
	}
	printf("-- Max dist : %f \n", max_dist);
	printf("-- Min dist : %f \n", min_dist);
	//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
	//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
	//-- small)
	//-- PS.- radiusMatch can also be used here.
	std::vector< DMatch > good_matches;
	

	for (int i = 0; i < descriptors_1.rows; i++)
	{
		if (matches[i].distance <= max(2 * min_dist, 0.02))
		{
			good_matches.push_back(matches[i]);
		}
	}

    if(good_matches.empty())
    {
        cout <<endl <<"No good matches was found, try again with higher threshold or different detection method" <<endl;
        return 0;
    }

	//-- Draw only "good" matches;
	Mat img_matches;
	drawMatches(img_1, keypoints_1, img_2, keypoints_2,
		good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
		vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	//-- Localize the object
	std::vector<Point2f> obj;
	std::vector<Point2f> scene;

	for (int i = 0; i < good_matches.size(); i++)
	{
		//-- Get the keypoints from the good matches
		obj.push_back(keypoints_1[good_matches[i].queryIdx].pt);
		scene.push_back(keypoints_2[good_matches[i].trainIdx].pt);
	}

	Mat H = findHomography(obj, scene, CV_RANSAC);

	//-- Get the corners from the image_1 ( the object to be "detected" )
	std::vector<Point2f> obj_corners(4);
	obj_corners[0] = cvPoint(0, 0);
    obj_corners[1] = cvPoint(img_1.cols, 0);
	obj_corners[2] = cvPoint(img_1.cols, img_1.rows);
    obj_corners[3] = cvPoint(0, img_1.rows);
	std::vector<Point2f> scene_corners(4);

	perspectiveTransform(obj_corners, scene_corners, H);

	//-- Draw lines between the corners (the mapped object in the scene - image_2 )
	line(img_matches, scene_corners[0] + Point2f(img_1.cols, 0), scene_corners[1] + Point2f(img_1.cols, 0), Scalar(255, 0, 0), 4);
	line(img_matches, scene_corners[1] + Point2f(img_1.cols, 0), scene_corners[2] + Point2f(img_1.cols, 0), Scalar(255, 0, 0), 4);
	line(img_matches, scene_corners[2] + Point2f(img_1.cols, 0), scene_corners[3] + Point2f(img_1.cols, 0), Scalar(255, 0, 0), 4);
	line(img_matches, scene_corners[3] + Point2f(img_1.cols, 0), scene_corners[0] + Point2f(img_1.cols, 0), Scalar(255, 0, 0), 4);

	//-- Show detected matches
	namedWindow("Good Matches", WINDOW_FREERATIO);
	imshow("Good Matches", img_matches);
	for (int i = 0; i < (int)good_matches.size(); i++)
	{
		printf("-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx);
	}

	waitKey(0);
	return 0;
}

void interactiveConsoleInput(Mat& src1, Mat& src2, DetectionMethod& detectionMethod, double& threshold)
{
    bool validInput;

    string path;
    int detectionMethodNum;
    do
    {
        try
        {
            cout << "Enter first image full path:" << endl;
            cin >> path;
            src1 = imread(path);

            if(src1.empty() || !src1.data)
            {
                cout  << endl << path << " is not a valid path! try again" << endl << endl;
                throw Exception();
            }

            cout << "Enter second image full:" << endl;
            cin >> path;
            src2 = imread(path);

            if(src2.empty() || !src2.data)
            {
                cout  << endl << path << " is not a valid path! try again" << endl << endl;
                throw Exception();
            }

            cout << "Choose detection method:" << endl <<"1)BRISK (More Accurate)"<<endl <<"2)ORB (Faster)" << endl;
            cin >> detectionMethodNum;
            detectionMethod = (DetectionMethod)detectionMethodNum;

            if(detectionMethod < 1 || detectionMethod > 2)
            {
                cout  << endl << "Not a valid option! choose one of the following next time:" << endl <<"1)BRISK (More Accurate)"<<endl <<"2) ORB (Faster)" << endl << endl;
                throw Exception();
            }
            cout << "Enter Threshold:" << endl;
            cin >> threshold;

            if(threshold < 0)
            {
                cout  << endl <<"The Threshold must be a positive number! " << endl;
                throw Exception();
            }
            validInput = true;
        }
        catch (Exception &exception1)
        {
           validInput = false;
        }
    }while(!validInput);
}