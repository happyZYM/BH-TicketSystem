#ifndef SNAP_SHOT_H
#define SNAP_SHOT_H
#include <string>
#include "map.hpp"
#include "storage/driver.h"
#include "vector.hpp"
void GenerateDiff(const std::string &old_file, const std::string &new_file, const std::string &diff_file);
void ApplyPatch(const std::string &old_file, const std::string &diff_file, const std::string &new_file,
                bool is_reverse);
/**
 * @brief SnapShotManager is a class to manage the snapshot of the data drivers.
 */

/**
The structure of the meta file is as follows:
[HEAD]
[ID1] [Anc1]
...
*/
class SnapShotManager {
  bool has_connected = false;
  bool has_set_meta_file = false;
  sjtu::vector<DataDriverBase *> drivers;
  std::string meta_file;

 public:
  // For safety and simplicity, we delete all the copy/move constructor and copy/move assignment operator. Please
  // manager it using smart pointer.
  SnapShotManager() = default;
  SnapShotManager(const SnapShotManager &) = delete;
  SnapShotManager(SnapShotManager &&) = delete;
  SnapShotManager &operator=(const SnapShotManager &) = delete;
  SnapShotManager &operator=(SnapShotManager &&) = delete;
  /**
   * @brief connect to the data drivers
   *
   * @warning please ensure that the data drivers are valid and the data drivers are not changed during the life cycle.
   * Meanwhile, the FileEntry collected from the data drivers should be valid during the life cycle.
   */
  inline void Connect(sjtu::vector<DataDriverBase *> drivers_) {
    if (has_connected) throw std::runtime_error("SnapShotManager has already connected to the data drivers");
    drivers = std::move(drivers_);
    has_connected = true;
  }
  inline void SetMetaFile(const std::string &meta_file_) {
    if (has_set_meta_file) throw std::runtime_error("SnapShotManager has already set the meta file");
    has_set_meta_file = true;
    meta_file = meta_file_;
    // check if the file exists
    FILE *f = fopen(meta_file.c_str(), "r");
    if (f == nullptr) {
      if (!has_connected)
        throw std::runtime_error(
            "SnapShotManager has not connected to the data drivers before initializing the repository");
      InitializeRepository();
    } else {
      fclose(f);
    }
  }
  void InitializeRepository();
  void CreateSnapShot(const std::string &snap_shot_ID);
  void SwitchToSnapShot(const std::string &snap_shot_ID);
  void RemoveSnapShot(const std::string &snap_shot_ID);
};
#endif  // SNAP_SHOT_H