#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace cv;

// edge - shows the results of edge detection


int main(int argc, char **argv)
{
  if(argc < 2) {
      std::cout << "./edge <in.mp4>" << std::endl;
      return 0;
  }

  Mat cur, cur_grey;
  VideoCapture cap(argv[1]);
  assert(cap.isOpened());
    
  while(true) {
    
  cap >> cur;
  
  if( cur.data == NULL ) {
    break;
  }

  int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  
  cvtColor(cur, cur_grey, COLOR_BGR2GRAY);
  
  // vector from prev to cur
  Mat canvas;

  Canny(cur_grey, canvas, 600.0, 600.0);
  
  resize(canvas, canvas, canvas.size()/2);
  
  imshow("edges", canvas);
  
  waitKey();
  
  }
  
  return 0;
}
