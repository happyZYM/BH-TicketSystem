#ifndef SNAP_SHOT_H
#define SNAP_SHOT_H
#include <string>
#include "map.hpp"
#include "vector.hpp"
class SnapShotManager {
 public:
  // For safety and simplicity, we delete all the copy/move constructor and copy/move assignment operator. Please
  // manager it using smart pointer.
  SnapShotManager(const SnapShotManager &) = delete;
  SnapShotManager(SnapShotManager &&) = delete;
  SnapShotManager &operator=(const SnapShotManager &) = delete;
  SnapShotManager &operator=(SnapShotManager &&) = delete;
};
#endif  // SNAP_SHOT_H