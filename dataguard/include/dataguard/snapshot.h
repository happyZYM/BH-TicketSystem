#ifndef SNAP_SHOT_H
#define SNAP_SHOT_H
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <string>
#include "list.hpp"
#include "map.hpp"
#include "storage/driver.h"
#include "vector.hpp"
void GenerateDiff(const std::string &old_file, const std::string &new_file, const std::string &diff_file);
void ApplyPatch(const std::string &old_file, const std::string &diff_file, const std::string &new_file,
                bool is_reverse);
void CopyFile(const std::string &src, const std::string &dst);
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
  std::shared_ptr<spdlog::logger> logger_ptr;
  struct WayEntry {
    std::string snap_ID;
    std::string diff_ID;
    bool is_reverse;
  };
  friend void ApplyLongChange(const std::string &old_file, const std::string &new_file,
                              const sjtu::vector<SnapShotManager::WayEntry> &way);
  inline sjtu::vector<WayEntry> FindWay(std::string dest) {
    if (!has_set_meta_file) {
      throw std::runtime_error("SnapShotManager has not set the meta file");
    }
    if (!has_connected) {
      throw std::runtime_error("SnapShotManager has not connected to the data drivers");
    }
    std::fstream fs(meta_file, std::ios::in);
    std::string HEAD;
    fs >> HEAD;
    std::string cur, anc;
    sjtu::map<std::string, sjtu::vector<std::string>> son_list;
    sjtu::map<std::string, std::string> get_anc;
    while (fs >> cur >> anc) {
      son_list[anc].push_back(cur);
      get_anc[cur] = anc;
    }
    if (son_list.find(dest) == son_list.end() && get_anc.find(dest) == get_anc.end()) {
      throw std::runtime_error("unable to find destination");
    }
    sjtu::vector<WayEntry> res;
    if (HEAD == dest) return res;
    sjtu::list<std::string> Q;
    sjtu::map<std::string, WayEntry> visit_record;
    Q.push_back(dest);
    while (!Q.empty()) {
      std::string cur = Q.front();
      Q.pop_front();
      if (get_anc.find(cur) != get_anc.end()) {
        std::string v = get_anc[cur];
        if (visit_record.find(v) == visit_record.end()) {
          visit_record[v] = {cur, cur, false};
          if (v == HEAD) goto ed;
          Q.push_back(v);
        }
      }
      if (son_list.find(cur) != son_list.end()) {
        auto &s_l = son_list[cur];
        for (size_t j = 0; j < s_l.size(); j++) {
          std::string v = s_l[j];
          if (visit_record.find(v) == visit_record.end()) {
            visit_record[v] = {cur, v, true};
            if (v == HEAD) goto ed;
            Q.push_back(v);
          }
        }
      }
    }
  ed:;
    std::string tmp = HEAD;
    while (tmp != dest) {
      res.push_back(visit_record[tmp]);
      tmp = visit_record[tmp].snap_ID;
    }
    return res;
  }
  inline void ApplyLongChange(const std::string &old_file, const std::string &new_file,
                              const sjtu::vector<SnapShotManager::WayEntry> &way, const std::string &file_name_base) {
    CopyFile(old_file, new_file);
    for (size_t i = 0; i < way.size(); i++) {
      if (logger_ptr) {
        logger_ptr->info("Applying diff {} to {} with inverse mark {}", file_name_base + "." + way[i].diff_ID + ".diff",
                         new_file, way[i].is_reverse);
      }
      ApplyPatch(new_file, file_name_base + "." + way[i].diff_ID + ".diff", new_file + ".tmp", way[i].is_reverse);
      remove(new_file.c_str());
      rename((new_file + ".tmp").c_str(), new_file.c_str());
      if (logger_ptr) {
        logger_ptr->info("Applied diff {} to {} with inverse mark {}", file_name_base + "." + way[i].diff_ID + ".diff",
                         new_file, way[i].is_reverse);
      }
    }
  }

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
  inline void SetLogger(const std::shared_ptr<spdlog::logger> &logger_ptr_) { logger_ptr = logger_ptr_; }
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
  void CheckOutFrontier();
  void SwitchToSnapShot(const std::string &snap_shot_ID);
  void RemoveSnapShot(const std::string &snap_shot_ID);
};
#endif  // SNAP_SHOT_H