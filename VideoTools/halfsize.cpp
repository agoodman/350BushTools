#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace cv;

// halfsize - exports input video to output video, scaled down by half


int main(int argc, char **argv)
{
  if(argc < 2) {
      std::cout << "./halfsize <in.mp4> <out.mp4>" << std::endl;
      return 0;
  }

  Mat cur;
  VideoCapture cap(argv[1]);
  assert(cap.isOpened());
  
  int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    
  std::string dstPath(argv[2]);

  VideoWriter writer;
  writer.open(dstPath.c_str(), cap.get(CV_CAP_PROP_FOURCC), cap.get(CV_CAP_PROP_FPS), Size(w/2,h/2), true);

  while(true) {

  cap >> cur;
  
  if( cur.data == NULL ) {
    break;
  }
  
  Mat canvas;
  cur.copyTo(canvas);
  
  resize(canvas, canvas, Size(w/2, h/2));

  writer << canvas;
  
  }
  
  return 0;
}
