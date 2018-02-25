#include <stdio.h>
#include <iostream>
#include <vector>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/stitching.hpp"

using namespace std;
using namespace cv;


void interactiveMenu(const string videoNames[], int numOfVideos, int& chosenOption, int& frameFreq);

void loadFramesAndShowVideo(int chosenOption, vector<Mat>& frames, int frameRate);

void fixFramesSize(vector<Mat>& frames);

void homographiesBetweenFollowingFrames(vector<Mat> &frames, vector<int> &dxs, vector<Mat> &homographies);

void checkImages(const Mat& gray_image1,const Mat& gray_image2);

vector< DMatch > detectExtractAndMatchFeatures( Ptr<Feature2D>& detector, Ptr<Feature2D>& extractor, Mat& image1, Mat& image2,
                                                vector< KeyPoint >& keypoints_object, vector< KeyPoint >& keypoints_scene, Mat& descriptors_object, Mat& descriptors_scene);

vector< DMatch > findGoodMatches(Mat& descriptors_object, vector< DMatch >& matches);

void mergeFrames(int positionOffset, vector<Mat> &frames, vector<int>& dxs, int sliceWidth, Mat& result, int height);

void fitSize(Mat& leftEye, Mat& rightEye);

void matToRed(Mat& inputFrame);

void matToCyan(Mat& inputFrame);

void createPanoramasAndAnaglyph(vector<Mat> &frames, vector<int> &dxs, int margin, int frameRate);

void createShowAndWritePanorama(vector<Mat> &frames, vector<int>& dxs, Mat& result, int& range, int margin, int frameRate);


constexpr int NUM_OF_VIDEOS = 5;

string VIDEO_PATHS[] = {"/Users/roie/Desktop/mountain.mov",
                         "/Users/roie/Desktop/gardenWithMoshe.mp4",
                         "/Users/roie/Desktop/playground3.mp4",
                         "/Users/roie/Desktop/insideHouse23.mp4","                                             "},

        VIDEO_NAMES[] = {"Mountain", "House Garden","Playground", "Living Room","Your Own Video"};

string vidName;

/**
 * Created by Roie Danino and Moshe Amini
 *
 */

int main()
{
    int margin = 6, frameFreq;
    int chosenOption;

    vector<Mat> frames;
    vector<int> dxs;
    vector<Mat> homographies;

    interactiveMenu(VIDEO_NAMES, NUM_OF_VIDEOS, chosenOption, frameFreq);

    loadFramesAndShowVideo(chosenOption, frames,frameFreq);

    fixFramesSize(frames);

    homographiesBetweenFollowingFrames(frames, dxs, homographies);

    createPanoramasAndAnaglyph(frames, dxs, margin, frameFreq);

    waitKey(0);
}

void interactiveMenu(const string videoNames[], int numOfVideos, int& chosenOption, int& frameFreq)
{
    bool stopLoop = false;
    string menu = "Please choose one of the following videos: ";
    int choice = 0;
    for (int i = 0; i < numOfVideos ; i++)
    {
        menu += " \n "+ to_string(i+1) + ") " + videoNames[i];
    }

    do
    {
        try {
            cout << menu << endl;
            cin >> choice;

            if (choice < 1 || choice > numOfVideos)
                throw Exception();

            if(choice == numOfVideos)
            {
                string path;
                cout <<"Please enter full path of your video:";
                cin >> path;
                VIDEO_PATHS[numOfVideos - 1] = path;

            }

            cout <<"Please Enter Frame Frequency in range: 1 (High Quality) - 10 (Fast)" << endl;
            cin >> frameFreq;

            if(frameFreq < 1 || frameFreq > 10)
                throw Exception();
            else
                stopLoop = true;
        }
        catch (Exception& exception) {
            cout << "Oops! Try Again!" << endl;
        }
    }while(!stopLoop);

    chosenOption = --choice;
}

void loadFramesAndShowVideo(int chosenOption, vector<Mat>& frames, int frameRate)
{
    const string windowName = "Input Video";
    Mat image;
    VideoCapture cap(VIDEO_PATHS[chosenOption]);

    vidName = VIDEO_NAMES[chosenOption];

    namedWindow(windowName, CV_WINDOW_FREERATIO);
    int counter = 0;

    int fps = (int)cap.get(CV_CAP_PROP_FPS);

    cout << fps << endl;

    while (true)
    {
        if (!cap.read(image))
            break;

        if (counter %  frameRate == 0)
            frames.push_back(image.clone());

        imshow(windowName, image);

        waitKey(fps);

        counter++;
    }
    destroyWindow(windowName);
    image.release();
    cap.release();
}

void fixFramesSize(vector<Mat>& frames)
{

    for (int i = 0; i < frames.size(); i++)
    {
        if (frames.at(i).cols > 1000 && frames.at(i).rows > 1000)
            resize(frames.at(i), frames.at(i), Size(frames.at(i).rows / 8, frames.at(i).cols / 8));
    }
}

void homographiesBetweenFollowingFrames(vector<Mat> &frames, vector<int> &dxs, vector<Mat> &homographies)
{
    Ptr<Feature2D> detector = xfeatures2d::SIFT::create();
    Ptr<Feature2D> extractor = xfeatures2d::SIFT::create();
    vector< DMatch > matches;
    Mat image1,image2,gray_image1, gray_image2, descriptors_object, descriptors_scene,H;

    for (int j = 0; j < frames.size() - 1; j++)
    {
        cout << j << endl;
        image1 = frames.at(j + 1);
        image2 = frames.at(j);

        cvtColor(image1, gray_image1, CV_RGB2GRAY);
        cvtColor(image2, gray_image2, CV_RGB2GRAY);

        checkImages(gray_image1, gray_image2);

        vector< KeyPoint > keypoints_object, keypoints_scene;

        matches = detectExtractAndMatchFeatures(detector,extractor,gray_image1,gray_image2,keypoints_object,keypoints_scene,descriptors_object,descriptors_scene);

        vector<DMatch> good_matches = findGoodMatches(descriptors_object, matches);

        std::vector< Point2f > obj;
        std::vector< Point2f > scene;

        cout << "good matches: " + to_string(good_matches.size()) << endl;

        for (int i = 0; i < good_matches.size(); i++)
        {
            obj.push_back(keypoints_object[good_matches[i].queryIdx].pt);
            scene.push_back(keypoints_scene[good_matches[i].trainIdx].pt);
        }

        H = findHomography(obj, scene, CV_RANSAC);

        cout << H << endl;

        int dx = (int)(H.at<double>(Point(2, 0)));
        if (dx < 0)
            dx *= -1;

        dxs.push_back(dx);
        homographies.push_back(H.clone());
    }

    image1.release();
    image2.release();
    gray_image1.release();
    gray_image2.release();
    descriptors_object.release();
    descriptors_scene.release();
    H.release();

}

void checkImages(const Mat& gray_image1,const Mat& gray_image2)
{
    if (!gray_image1.data || !gray_image2.data)
    {
        std::cout << " --(!) Error reading images " << std::endl;
        exit(1);
    }
}

vector< DMatch > detectExtractAndMatchFeatures( Ptr<Feature2D>& detector, Ptr<Feature2D>& extractor, Mat& image1, Mat& image2,
                                                vector< KeyPoint >& keypoints_object, vector< KeyPoint >& keypoints_scene, Mat& descriptors_object, Mat& descriptors_scene)
{
    detector->detect(image1, keypoints_object);
    detector->detect(image2, keypoints_scene);

    extractor->compute(image1, keypoints_object, descriptors_object);
    extractor->compute(image2, keypoints_scene, descriptors_scene);

    BFMatcher matcher;
    vector< DMatch > matches;

    matcher.match(descriptors_object, descriptors_scene, matches);

    return matches;
}

vector< DMatch > findGoodMatches(Mat& descriptors_object, vector< DMatch >& matches)
{
    double max_dist = 0;
    double min_dist = 100;

    for (int i = 0; i < descriptors_object.rows; i++)
    {
        double dist = matches[i].distance;
        if (dist < min_dist) min_dist = dist;
        if (dist > max_dist) max_dist = dist;
    }

    printf("-- Max dist : %f \n", max_dist);
    printf("-- Min dist : %f \n", min_dist);

    vector<DMatch> good_matches;
    int good = 0;
    double count = 2;
    cout <<"Matches: " << descriptors_object.rows << endl;
    while (good < 50)
    {
        good_matches.clear();
        for (int i = 0; i < descriptors_object.rows; i++)
        {
            if (matches[i].distance <= count * (min_dist + 1))
            {
                good_matches.push_back(matches[i]);
            }
        }
        good = good_matches.size();
        count += 0.5;
    }
    return good_matches;
}

void createShowAndWritePanorama(vector<Mat> &frames, vector<int>& dxs, Mat& result, int& range, int margin, int frameRate)
{
    frames.at(0).copyTo(result(Rect(0, 0, frames.at(0).cols, frames.at(0).rows)));
    for (int i = 0; i < dxs.size(); i++)
    {
        Mat temp = frames.at(i + 1).clone();
        Mat temp2 = temp(Rect(temp.cols - dxs.at(i), 0, dxs.at(i), temp.rows));
        temp2.copyTo(result(Rect(frames.at(0).cols + range, 0, temp2.cols, temp.rows)));
        range += dxs.at(i) - margin;
        temp.release();
    }
    imshow("Panorama", result);
    imwrite("/Users/roie/Desktop/panorama "+ vidName + to_string(frameRate) + " .jpg", result);
}

void mergeFrames(int positionOffset, vector<Mat> &frames, vector<int>& dxs, int sliceWidth, Mat& result, int height)
{
    int atFrame, howMuch, range = sliceWidth;
    for (int i = 1; i < frames.size(); i++)
    {
        if (dxs.at(i - 1) < sliceWidth)
        {
            atFrame = positionOffset + sliceWidth - dxs.at(i - 1);
            howMuch = dxs.at(i - 1);
        }
        else
        {
            atFrame = positionOffset;
            howMuch = sliceWidth;
        }
        Mat temp = frames.at(i)(Rect(atFrame, 0, howMuch, height));
        temp.copyTo(result(Rect(range, 0, howMuch, height)));
        range += howMuch;
    }
}

void fitSize(Mat& leftEye, Mat& rightEye)
{
    if (leftEye.cols <= rightEye.cols)
    {
        rightEye = rightEye(Rect(0, 0, leftEye.cols, rightEye.rows));
    }
    else
    {
        leftEye = leftEye(Rect(0, 0, rightEye.cols, leftEye.rows));
    }
}

void matToRed(Mat& inputFrame)
{
    //Only Red
    for (int i = 0; i < inputFrame.cols; i++)
    {
        for (int j = 0; j < inputFrame.rows; j++)
        {
            (inputFrame.at<Vec3b>(Point(i, j))).val[0] = 0;
            (inputFrame.at<Vec3b>(Point(i, j))).val[1] = 0;
        }
    }
}

void matToCyan(Mat& inputFrame)
{
    //Only Blue and Green - cyan
    for (int i = 0; i < inputFrame.cols; i++)
    {
        for (int j = 0; j < inputFrame.rows; j++)
        {
            (inputFrame.at<Vec3b>(Point(i, j))).val[2] = 0;
        }
    }
}

void createPanoramasAndAnaglyph(vector<Mat> &frames, vector<int> &dxs, int margin, int frameRate)
{
    int total_dxs = 0;
    for (int i = 0; i < dxs.size(); i++)
    {
        total_dxs += dxs.at(i);
    }

    Mat result(frames.at(0).rows, frames.at(0).cols + total_dxs, 16);
    int range = -margin;

    createShowAndWritePanorama(frames, dxs, result, range, margin, frameRate);

    Mat firstFrame = frames.at(0);
    Mat panorama = result;
    
    int width = firstFrame.cols, height = firstFrame.rows;

    int sliceWidth = width / 7;
    
    int leftPosition = 2 * sliceWidth;
    int rightPosition = sliceWidth;

    int widthLeft = total_dxs + sliceWidth, widthRight = total_dxs + sliceWidth;
    
    Mat rightEye(height, widthRight, 16), leftEye(height, widthLeft, 16);

    mergeFrames(rightPosition, frames, dxs, sliceWidth, rightEye, height);

    imshow("Right", rightEye);
    imwrite("/Users/roie/Desktop/right "+  vidName+ to_string(frameRate) +" .jpg", rightEye);
    
    mergeFrames(leftPosition, frames, dxs, sliceWidth, leftEye, height);
    imshow("Left", leftEye);
    imwrite("/Users/roie/Desktop/left "+ vidName + to_string(frameRate) + " .jpg", leftEye);
    
    rightEye = rightEye(Rect(leftPosition - rightPosition, 0, rightEye.cols - (leftPosition - rightPosition), rightEye.rows));

    fitSize(leftEye, rightEye);

    matToCyan(rightEye);

    matToRed(leftEye);

    Mat stereo = leftEye + rightEye;
    
    imwrite("/Users/roie/Desktop/stereo " + vidName + to_string(frameRate) + " .jpg", stereo);
    
    imshow("Stereo", stereo);
}


