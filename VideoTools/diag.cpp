#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include "fileTools.hpp"

using namespace cv;

// chunk - selects a frame from a set of video clips and generates a new clip from the sequence

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cout << "./diag <src-path> <output.mp4>" << std::endl;
        return 0;
    }

    std::string srcPath(argv[1]);
    std::string dstFileName(argv[2]);
    
    // read files from src path
    std::vector<std::string> srcFiles = FilesInDirectory(srcPath);
    
    std::string setupFileName = srcPath + "/" + srcFiles[0];
    std::cout << "Reading setup data from " << setupFileName << std::endl;
    
    VideoCapture setup(setupFileName);
    assert(setup.isOpened());
    int w = setup.get(CV_CAP_PROP_FRAME_WIDTH);
    int h = setup.get(CV_CAP_PROP_FRAME_HEIGHT);
    int frameCount = setup.get(CV_CAP_PROP_FRAME_COUNT);
    int fps = setup.get(CV_CAP_PROP_FPS);
    int fourcc = setup.get(CV_CAP_PROP_FOURCC);

    std::cout << w << "x" << h << " IN frames: " << frameCount << " fps: " << fps << std::endl;
    
    float duration = (float)frameCount / (float)fps;
    int outFrames = srcFiles.size();
    int outStep = (int)floor((float)frameCount / (float)outFrames);
    int outFps = (int)((float)outFrames / duration);
    
    std::cout << " OUT frames: " << outFrames << " fps: " << outFps << " step: " << outStep << std::endl;
    
    VideoWriter writer;
    writer.open(dstFileName.c_str(), fourcc, outFps, Size(w,h), true);
    
    int offset = frameCount - outStep - outFrames * outStep;
    int k = - 1;
    const std::string lastFile = srcFiles.back();
    srcFiles.pop_back();
    for (const std::string& fileName : srcFiles) {
      VideoCapture cap(srcPath + "/" + fileName);
      if(cap.isOpened()) {
        if(cap.set(CV_CAP_PROP_POS_FRAMES, offset + outStep*(++k))) {
          Mat frame;
          cap >> frame;
          if(frame.data == NULL) {
              break;
          }
          writer << frame;
          std::cout << "\rprogress: " << (int)(100.0f*(float)k / (float)srcFiles.size()) << "\% (" << (offset + outStep*k) << " / " << frameCount << ")" << std::flush;
        }
        else {
          std::cout << "Unable to seek to frame " << (offset + outStep*k) << " in " << fileName << std::endl;
        }
      }
      else {
        std::cout << "Unable to open " << fileName << std::endl;
      }
    }
    VideoCapture cap(srcPath + "/" + lastFile);
    cap.set(CV_CAP_PROP_POS_FRAMES, cap.get(CV_CAP_PROP_FRAME_COUNT)-1);
    Mat frame;
    cap >> frame;
    if( frame.data != NULL ) {
      writer << frame;
    }
    std::cout << "\rProcessed " << k << " frames                  " << std::endl;

    return 0;
}
