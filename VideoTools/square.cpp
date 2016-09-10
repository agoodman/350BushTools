#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "fileTools.hpp"

using namespace cv;

// square - exports a grid of frames as ii-jj numbered files in the output directory


bool IsVideoClip(const std::string& filePath)
{
  if (filePath.length() >= 4) {
    return (0 == filePath.compare(filePath.length() - 4, 4, ".mp4"));
  } else {
    return false;
  }
}

void WriteFrames(const std::string& videoClipPath,
                 const std::string& outPath,
                 const uint8_t gridSize,
                 const uint8_t i)
{
  Mat cur;
  VideoCapture clip(videoClipPath);
  assert(clip.isOpened());
  
  int frameCount = clip.get(CV_CAP_PROP_FRAME_COUNT);
  float outStep = (float)frameCount / (float)gridSize;

  // generate indices for exactly gridSize frames
  for (int k=0;k<gridSize;k++) {
    int index = outStep * k;
    
    if(clip.set(CV_CAP_PROP_POS_FRAMES, index)) {
      clip >> cur;
      
      if( cur.data == NULL ) {
        break;
      }
      
      std::stringstream indexString;
      indexString << std::setfill('0') << std::setw(2) << (int)i;
      indexString << "-";
      indexString << std::setfill('0') << std::setw(2) << k;
      std::string filePath(outPath + "/" + indexString.str() + ".jpg");
      imwrite(filePath, cur);
    }
  }
}

int main(int argc, char **argv)
{
  if(argc < 2) {
    std::cout << "./square <in-path> <out-path>" << std::endl;
    return 0;
  }
  
  std::string srcPath(argv[1]);
  std::string dstPath(argv[2]);
  std::vector<std::string> files = FilesInDirectory(srcPath);
  
  uint8_t gridSize = 0;
  for (const auto& file : files) {
    if( IsVideoClip(file) ) {
      gridSize++;
    }
  }
  
  int k = 0;
  for (const auto& file : files) {
    if( IsVideoClip(file) ) {
      WriteFrames(srcPath+"/"+file, dstPath, gridSize, k++);
    }
  }
  
  return 0;
}


