#ifndef SNAP_SHOT_H
#define SNAP_SHOT_H
#include <string>
#include "map.hpp"
#include "storage/driver.h"
#include "vector.hpp"
class SnapShotManager {
  bool has_connected = false;
  sjtu::vector<DataDriverBase *> drivers;

 public:
  // For safety and simplicity, we delete all the copy/move constructor and copy/move assignment operator. Please
  // manager it using smart pointer.
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
};
#endif  // SNAP_SHOT_H