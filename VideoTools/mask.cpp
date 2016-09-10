#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace cv;

// mask - shows the results of top-right and right-half masked feature detection


int main(int argc, char **argv)
{
  if(argc < 2) {
      std::cout << "./mask <in.mp4>" << std::endl;
      return 0;
  }

  Mat cur, cur_grey, prev, prev_grey;
  VideoCapture cap(argv[1]);
  assert(cap.isOpened());
    
  // cap.set(CV_CAP_PROP_POS_FRAMES, cap.get(CV_CAP_PROP_FRAME_COUNT)/3);
  while(true) {
    
  cap >> prev;
  
  if( prev.data == NULL ) {
    break;
  }

  Mat mask(prev.size(), CV_8UC1);
  mask.setTo(Scalar::all(0));
  Scalar maskColor(255,255,255);
  int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  // top-half
  // rectangle(mask, Point(0,0), Point(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT)*0.5), maskColor, CV_FILLED, 8, 0);
  // top-right
  // rectangle(mask, Point(cap.get(CV_CAP_PROP_FRAME_WIDTH)*0.5,0), Point(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT)*0.5), maskColor, CV_FILLED, 8, 0);
  // right 50%
  // rectangle(mask, Point(cap.get(CV_CAP_PROP_FRAME_WIDTH)*0.6,0), Point(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT)), maskColor, CV_FILLED, 8, 0);
  // center 50%
  // rectangle(mask, Point(0.25*w,0.25*h), Point(0.75*w, 0.75*h), maskColor, CV_FILLED, 8, 0);
  // full screen
  rectangle(mask, Point(0,0), Point(w, h), maskColor, CV_FILLED, 8, 0);  
  
  cvtColor(prev, prev_grey, COLOR_BGR2GRAY);

  cap >> cur;
  
  if( cur.data == NULL ) {
    break;
  }

  cvtColor(cur, cur_grey, COLOR_BGR2GRAY);
  
  // vector from prev to cur
  std::vector <Point2f> prev_corner, cur_corner;
  std::vector <uchar> status;
  std::vector <float> err;

  goodFeaturesToTrack(prev_grey, prev_corner, 1000, 0.001, 5, mask, 5, true, 0.05);
  goodFeaturesToTrack(cur_grey, cur_corner, 1000, 0.001, 5, mask, 5, true, 0.05);
  
  calcOpticalFlowPyrLK(prev_grey, cur_grey, prev_corner, cur_corner, status, err);
  
  Mat cur_channel[3], prev_channel[3];
  split(cur, cur_channel);
  split(prev, prev_channel);
  cur_channel[0] = Mat::zeros(cur.rows, cur.cols, CV_8UC1);
  cur_channel[1] = Mat::zeros(cur.rows, cur.cols, CV_8UC1);
  prev_channel[0] = Mat::zeros(prev.rows, prev.cols, CV_8UC1);
  prev_channel[2] = Mat::zeros(prev.rows, prev.cols, CV_8UC1);

  Mat cur_mix, prev_mix;
  merge(cur_channel, 3, cur_mix);
  merge(prev_channel, 3, prev_mix);

  Mat canvas = cur_mix + prev_mix;
  
  for (int k=0;k<prev_corner.size();k++) {
    circle(canvas, cur_corner[k], 2, Scalar(255,255,128), -1, 8, 0);
    circle(canvas, prev_corner[k], 2, Scalar(255,128,255), -1, 8, 0);
    double dist = norm(cur_corner[k]-prev_corner[k]);
    Point2f avg = (cur_corner[k] + prev_corner[k]) / 2.0 - Point2f(cap.get(CV_CAP_PROP_FRAME_WIDTH)/2.0, cap.get(CV_CAP_PROP_FRAME_HEIGHT)/2.0);
    if( status[k] && dist < 0.1*norm(avg)) {
      arrowedLine(canvas, prev_corner[k], cur_corner[k], Scalar(255,0,0), 2, 8, 0);
    }
  }
  
  resize(canvas, canvas, canvas.size()/2);
  
  imshow("features", canvas);
  
  waitKey();
  
  }
  
  return 0;
}
