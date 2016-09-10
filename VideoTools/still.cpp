#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <stdlib.h>
#include <string.h>

using namespace cv;

// still - exports each frame as a numbered file in the output directory


int main(int argc, char **argv)
{
  if(argc < 2) {
    std::cout << "./still <in.mp4> <out-path>" << std::endl;
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

    std::stringstream indexString;
    indexString << std::setfill('0') << std::setw(2) << k;
    imwrite(dstPath + "/" + indexString.str() + ".jpg", cur);
  
    k++;
  
  }
  
  return 0;
}
