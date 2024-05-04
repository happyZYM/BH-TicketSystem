#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include "storage/disk_manager.h"
#include "vector.hpp"
class DataDriverBase {
 public:
  struct FileEntry {
    std::string identifier;
    std::string path;
    DiskManager *disk_manager;
  };
  DataDriverBase() = default;
  virtual ~DataDriverBase() = default;
  virtual sjtu::vector<FileEntry> ListFiles() = 0;
  virtual void Flush() = 0;
};
#endif  // DRIVER_H