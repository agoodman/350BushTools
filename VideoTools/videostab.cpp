#include <opencv2/opencv.hpp>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace std;
using namespace cv;

// This video stablisation smooths the global trajectory using a sliding average window

const int DEFAULT_SMOOTHING_RADIUS = 30; // In frames. The larger the more stable the video, but less reactive to sudden panning
const int HORIZONTAL_BORDER_CROP = 20; // In pixels. Crops the border to reduce the black borders from stabilisation being too noticeable.

// 1. Get previous to current frame transformation (dx, dy, da) for all frames
// 2. Accumulate the transformations to get the image trajectory
// 3. Smooth out the trajectory using an averaging window
// 4. Generate new set of previous to current transform, such that the trajectory ends up being the same as the smoothed trajectory
// 5. Apply the new transformation to the video

struct TransformParam
{
    TransformParam() {}
    TransformParam(double _dx, double _dy, double _da) {
        dx = _dx;
        dy = _dy;
        da = _da;
    }

    double dx;
    double dy;
    double da; // angle
};

struct Trajectory
{
    Trajectory() {}
    Trajectory(double _x, double _y, double _a) {
        x = _x;
        y = _y;
        a = _a;
    }

    double x;
    double y;
    double a; // angle
};

int main(int argc, char **argv)
{
    if(argc < 3) {
        cout << "./videostab <in.mp4> <out.mp4> [smoothing-radius]" << endl;
        return 0;
    }

    // For further analysis
    ofstream out_transform("prev_to_cur_transformation.txt");
    ofstream out_trajectory("trajectory.txt");
    ofstream out_smoothed_trajectory("smoothed_trajectory.txt");
    ofstream out_new_transform("new_prev_to_cur_transformation.txt");

    std::string smoothingRadius(argv[3]);
    int kSmoothingRadius = std::strtol(smoothingRadius.c_str(), NULL, 10);
    if( kSmoothingRadius == 0 ) {
      kSmoothingRadius = DEFAULT_SMOOTHING_RADIUS;
    }
    cout << "smoothing radius: " << kSmoothingRadius << endl;
    
    VideoCapture* cap = new VideoCapture(argv[1]);
    assert(cap->isOpened());

    Mat cur, cur_grey;
    Mat prev, prev_grey;

    *cap >> prev;
    cvtColor(prev, prev_grey, COLOR_BGR2GRAY);

    // Step 1 - Get previous to current frame transformation (dx, dy, da) for all frames
    vector <TransformParam> prev_to_cur_transform; // previous to current

    int k=1;
    int max_frames = cap->get(CV_CAP_PROP_FRAME_COUNT);
    Mat last_T;
    Mat mask(last_T.size(), CV_8UC1);
    mask.setTo(Scalar::all(0));
    Scalar maskColor(255,255,255);
    int w = cap->get(CV_CAP_PROP_FRAME_WIDTH);
    int h = cap->get(CV_CAP_PROP_FRAME_HEIGHT);
    // top-half
    // rectangle(mask, Point(0,0), Point(cap->get(CV_CAP_PROP_FRAME_WIDTH), cap->get(CV_CAP_PROP_FRAME_HEIGHT)*0.5), maskColor, CV_FILLED, 8, 0);
    // right 40%
    // rectangle(mask, Point(cap->get(CV_CAP_PROP_FRAME_WIDTH)*0.6,0), Point(cap->get(CV_CAP_PROP_FRAME_WIDTH), cap->get(CV_CAP_PROP_FRAME_HEIGHT)), maskColor, CV_FILLED, 8, 0);
    // center 50%
    rectangle(mask, Point(0.25*w,0.25*h), Point(0.75*w, 0.75*h), maskColor, CV_FILLED, 8, 0);
    
    // cout << "Computing transforms for " << max_frames << " frames" << endl;
    
    while(true) {
        *cap >> cur;

        if(cur.data == NULL) {
            break;
        }

        cvtColor(cur, cur_grey, COLOR_BGR2GRAY);

        // vector from prev to cur
        vector <Point2f> prev_corner, cur_corner;
        vector <Point2f> prev_corner2, cur_corner2;
        vector <uchar> status;
        vector <float> err;

        goodFeaturesToTrack(prev_grey, prev_corner, 200, 0.001, 30, mask);
        calcOpticalFlowPyrLK(prev_grey, cur_grey, prev_corner, cur_corner, status, err);

        // weed out bad matches
        for(size_t i=0; i < status.size(); i++) {
          Point2f avg = (cur_corner[i] + prev_corner[i]) / 2.0 - Point2f(cap->get(CV_CAP_PROP_FRAME_WIDTH)/2.0, cap->get(CV_CAP_PROP_FRAME_HEIGHT)/2.0);
          if( status[i] && norm(cur_corner[i]-prev_corner[i]) < 0.5*norm(avg)) {
                prev_corner2.push_back(prev_corner[i]);
                cur_corner2.push_back(cur_corner[i]);
            }
        }

        // translation + rotation only
        Mat T = estimateRigidTransform(prev_corner2, cur_corner2, false); // false = rigid transform, no scaling/shearing

        // in rare cases no transform is found. We'll just use the last known good transform.
        if(T.data == NULL) {
          last_T.copyTo(T);
        }
        else {
          T.copyTo(last_T);
        }

        double dx, dy, da;
        dx = dy = da = 0.0;
        
        if( T.rows > 0 && T.cols > 0 ) {
          
        // decompose T
        dx = T.at<double>(0,2);
        dy = T.at<double>(1,2);
        da = atan2(T.at<double>(1,0), T.at<double>(0,0));

        }
        
        prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

        out_transform << k << " " << dx << " " << dy << " " << da << endl;
        

        cur.copyTo(prev);
        cur_grey.copyTo(prev_grey);

        // cout << "Frame: " << k << "/" << max_frames << " - good optical flow: " << prev_corner2.size() << endl;
        k++;
    }

    // Step 2 - Accumulate the transformations to get the image trajectory

    // Accumulated frame to frame transform
    double a = 0;
    double x = 0;
    double y = 0;

    vector <Trajectory> trajectory; // trajectory at all frames

    for(size_t i=0; i < prev_to_cur_transform.size(); i++) {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;

        trajectory.push_back(Trajectory(x,y,a));

        out_trajectory << (i+1) << " " << x << " " << y << " " << a << endl;
    }

    // Step 3 - Smooth out the trajectory using an averaging window
    vector <Trajectory> smoothed_trajectory; // trajectory at all frames

    for(size_t i=0; i < trajectory.size(); i++) {
        double sum_x = 0;
        double sum_y = 0;
        double sum_a = 0;
        int count = 0;

        for(int j=-kSmoothingRadius; j <= kSmoothingRadius; j++) {
            if((int)i+j >= 0 && (int)i+j < trajectory.size()) {
                sum_x += trajectory[i+j].x;
                sum_y += trajectory[i+j].y;
                sum_a += trajectory[i+j].a;

                count++;
            }
        }

        double avg_a = sum_a / count;
        double avg_x = sum_x / count;
        double avg_y = sum_y / count;

        smoothed_trajectory.push_back(Trajectory(avg_x, avg_y, avg_a));

        out_smoothed_trajectory << (i+1) << " " << avg_x << " " << avg_y << " " << avg_a << endl;
    }

    // Step 4 - Generate new set of previous to current transform, such that the trajectory ends up being the same as the smoothed trajectory
    vector <TransformParam> new_prev_to_cur_transform;

    // Accumulated frame to frame transform
    a = 0;
    x = 0;
    y = 0;

    for(size_t i=0; i < prev_to_cur_transform.size(); i++) {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;

        // target - current
        double diff_x = smoothed_trajectory[i].x - x;
        double diff_y = smoothed_trajectory[i].y - y;
        double diff_a = smoothed_trajectory[i].a - a;

        double dx = prev_to_cur_transform[i].dx + diff_x;
        double dy = prev_to_cur_transform[i].dy + diff_y;
        double da = prev_to_cur_transform[i].da + diff_a;

        new_prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

        out_new_transform << (i+1) << " " << dx << " " << dy << " " << da << endl;
    }

    // Step 5 - Apply the new transformation to the video
    delete cap;
    cap = new VideoCapture(argv[1]);
    Mat T(2,3,CV_64F);

    int vert_border = HORIZONTAL_BORDER_CROP * prev.rows / prev.cols; // get the aspect ratio correct

    VideoWriter writer;
    writer.open(argv[2], cap->get(CV_CAP_PROP_FOURCC), cap->get(CV_CAP_PROP_FPS), Size(cap->get(CV_CAP_PROP_FRAME_WIDTH),cap->get(CV_CAP_PROP_FRAME_HEIGHT)), true);
    
    k=0;
    while(k < max_frames-1) { // don't process the very last frame, no valid transform
        *cap >> cur;

        if(cur.data == NULL) {
            break;
        }

        T.at<double>(0,0) = cos(new_prev_to_cur_transform[k].da);
        T.at<double>(0,1) = -sin(new_prev_to_cur_transform[k].da);
        T.at<double>(1,0) = sin(new_prev_to_cur_transform[k].da);
        T.at<double>(1,1) = cos(new_prev_to_cur_transform[k].da);

        T.at<double>(0,2) = new_prev_to_cur_transform[k].dx;
        T.at<double>(1,2) = new_prev_to_cur_transform[k].dy;

        Mat cur2;

        warpAffine(cur, cur2, T, cur.size());

        cur2 = cur2(Range(vert_border, cur2.rows-vert_border), Range(HORIZONTAL_BORDER_CROP, cur2.cols-HORIZONTAL_BORDER_CROP));

        // Resize cur2 back to cur size, for better side by side comparison
        resize(cur2, cur2, cur.size());

        writer.write(cur2);
        
        if( false ) {
        // Now draw the original and stablised side by side for coolness
        Mat canvas = Mat::zeros(cur.rows, cur.cols*2+10, cur.type());

        cur.copyTo(canvas(Range::all(), Range(0, cur2.cols)));
        cur2.copyTo(canvas(Range::all(), Range(cur2.cols+10, cur2.cols*2+10)));

        // If too big to fit on the screen, then scale it down by 2, hopefully it'll fit :)
        // if(canvas.cols > 1920) {
            resize(canvas, canvas, Size(canvas.cols/2, canvas.rows/2));
        // }

        imshow("before and after", canvas);

        //char str[256];
        //sprintf(str, "images/%08d.jpg", k);
        //imwrite(str, canvas);

        waitKey(20);
        }

        k++;
    }

    delete cap;

    return 0;
}
