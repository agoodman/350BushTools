#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace cv;

// chunk - selects a frame from a set of video clips and generates a new clip from the sequence

std::vector<std::string> FilesInDirectory(const std::string& path)
{
  std::vector<std::string> files;
  
  DIR* listingDir = opendir(path.c_str());
  if( listingDir ) {
    struct dirent *info;
    while( (info = readdir(listingDir)) ) {
      if( info->d_type==DT_REG ) {
        std::string fileName(info->d_name);
        files.push_back(fileName);
      }
    }
    closedir(listingDir);
  }
  
  return files;
}

int main(int argc, char **argv)
{
    if(argc < 3) {
        std::cout << "./chunk <src-path> <frame-index> <output.mp4>" << std::endl;
        return 0;
    }

    std::string srcPath(argv[1]);
    uint32_t frameIndex = std::stoi(argv[2]);
    std::string dstFileName(argv[3]);
    
    // read files from src path
    std::vector<std::string> srcFiles = FilesInDirectory(srcPath);
    
    std::string setupFileName = srcPath + "/" + srcFiles[0];
    std::cout << "Reading setup data from " << setupFileName << std::endl;
    
    VideoCapture setup(setupFileName);
    assert(setup.isOpened());
    int w = setup.get(CV_CAP_PROP_FRAME_WIDTH);
    int h = setup.get(CV_CAP_PROP_FRAME_HEIGHT);
    int frameCount = setup.get(CV_CAP_PROP_FRAME_COUNT);
    int inFps = setup.get(CV_CAP_PROP_FPS);
    int fourcc = setup.get(CV_CAP_PROP_FOURCC);

    std::cout << w << "x" << h << " frames: " << frameCount << " fps: " << inFps << std::endl;
    
    float duration = (float)frameCount / (float)inFps;
    int outFps = (int)((float)srcFiles.size() / duration);
    
    std::cout << "Output duration: " << duration << " frames: " << srcFiles.size() << " fps: " << outFps << std::endl;
    
    VideoWriter writer;
    writer.open(dstFileName.c_str(), fourcc, outFps, Size(w,h), true);
    int k = 0;
    for (const std::string& fileName : srcFiles) {
      VideoCapture cap(srcPath + "/" + fileName);
      if(cap.isOpened()) {
        if(cap.set(CV_CAP_PROP_POS_FRAMES, frameIndex)) {
          Mat frame;
          cap >> frame;
          writer << frame;
          std::cout << "\rprogress: " << (int)(100.0f*(float)(++k)/(float)srcFiles.size()) << "\%" << std::flush;
        }
        else {
          std::cout << "Unable to seek to frame " << frameIndex << " in " << fileName << std::endl;
        }
      }
      else {
        std::cout << "Unable to open " << fileName << std::endl;
      }
    }
    std::cout << "\r" << std::flush;

    return 0;
}
