//
//  fileTools.cpp
//  VideoTools
//
//  Created by Aubrey Goodman on 9/10/16.
//  Copyright © 2016 Aubrey Goodman. All rights reserved.
//

#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>


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

