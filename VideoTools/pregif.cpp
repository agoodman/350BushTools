#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace cv;

// pregif - exports each frame as a numbered file in the output directory, scaled down by half


int main(int argc, char **argv)
{
  if(argc < 2) {
      std::cout << "./pregif <in.mp4> <out-path>" << std::endl;
      return 0;
  }

  Mat cur;
  VideoCapture cap(argv[1]);
  assert(cap.isOpened());
    
  std::string dstPath(argv[2]);
  int k = 0;
  while(true) {

  cap >> cur;
  
  if( cur.data == NULL ) {
    break;
  }
  
  Mat canvas;
  cur.copyTo(canvas);
  
  resize(canvas, canvas, Size(canvas.cols/4, canvas.rows/4));

  imwrite(dstPath + "/" + std::to_string(k) + ".jpg", canvas);
  
  k++;
  
  }
  
  return 0;
}
