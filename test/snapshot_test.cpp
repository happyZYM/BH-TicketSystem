#include <gtest/gtest.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <deque>
#include <map>
#include <random>
#include <set>
#include "dataguard/dataguard.h"
#include "storage/disk_map.hpp"

namespace SnapShotTest {
template <typename Key>
struct KeysContainerForGenerator {
  std::deque<Key> keys_list;
  std::set<Key> keys_set;
  bool IsIn(const Key &key) const { return keys_set.find(key) != keys_set.end(); }
  template <typename random_generator_t>
  Key GetRandomKey(random_generator_t &rnd) {
    return keys_list[rnd() % keys_list.size()];
  }
  void AddKey(const Key &key) {
    if (IsIn(key)) return;
    keys_list.insert(std::lower_bound(keys_list.begin(), keys_list.end(), key), key);
    keys_set.insert(key);
  }
  void RemoveKey(const Key &key) {
    if (!IsIn(key)) return;
    keys_list.erase(std::lower_bound(keys_list.begin(), keys_list.end(), key));
    keys_set.erase(key);
  }
  size_t Size() const { return keys_list.size(); }
};
}  // namespace SnapShotTest
TEST(Hello, World) { return; }

TEST(Basic, DiskMap) {
  using namespace SnapShotTest;
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  KeysContainerForGenerator<int> keys_container;
  std::map<int, int> std_map;
  remove("/tmp/index.db");
  remove("/tmp/data.db");
  const int total_opts = 1000000;
  {
    DiskMap<int, int> disk_map("index", "/tmp/index.db", "data", "/tmp/data.db");
    for (int i = 0; i < total_opts; i++) {
      int opt_id = rnd() % 100;
      if (opt_id <= 40) {
        if (keys_container.Size() > 0 && rnd() % 5 <= 2) {
          // overrite and existing key
          int key = keys_container.GetRandomKey(rnd);
          int val = rnd() % 1000000;
          std_map[key] = val;
          disk_map.Put(key, val);
        } else {
          // insert a new key
          int key = rnd() % 1000000;
          int val = rnd() % 1000000;
          keys_container.AddKey(key);
          std_map[key] = val;
          disk_map.Put(key, val);
        }
      } else if (opt_id <= 60) {
        if (keys_container.Size() > 0 && rnd() % 5 <= 2) {
          // delete an existing key
          int key = keys_container.GetRandomKey(rnd);
          keys_container.RemoveKey(key);
          std_map.erase(key);
          disk_map.Remove(key);
        } else {
          // delete a non-existing key
          int key = rnd() % 1000000;
          keys_container.RemoveKey(key);
          std_map.erase(key);
          disk_map.Remove(key);
        }
      } else {
        if (keys_container.Size() == 0) continue;
        int key = keys_container.GetRandomKey(rnd);
        int val = disk_map.Get(key);
        if (std_map.find(key) == std_map.end()) {
          ASSERT_EQ(val, -1);
        } else {
          ASSERT_EQ(val, std_map[key]);
        }
      }
    }
  }
}

TEST(Basic, T1) {
  remove("/tmp/1.dat");
  remove("/tmp/2.dat");
  remove("/tmp/diff.dat");
  remove("/tmp/3.dat");
  remove("/tmp/4.dat");
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len_s1 = 10000;
  const int str_len_s2 = 9900;
  char s1[str_len_s1], s2[str_len_s2];
  for (int i = 0; i < str_len_s1 - 1; i++) {
    s1[i] = 'a' + rnd() % 26;
  }
  s1[str_len_s1 - 1] = '\0';
  memcpy(s2, s1, str_len_s2);
  for (int i = 0; i < str_len_s2 - 1; i++) {
    if (i >= str_len_s1) {
      s2[i] = 'a' + rnd() % 26;
      continue;
    }
    if (rnd() % 3 == 0) {
      s2[i] = 'a' + rnd() % 26;
    }
  }
  s2[str_len_s2 - 1] = '\0';
  // write to file
  FILE *fp = fopen("/tmp/1.dat", "wb");
  fwrite(s1, 1, str_len_s1, fp);
  fclose(fp);
  fp = fopen("/tmp/2.dat", "wb");
  fwrite(s2, 1, str_len_s2, fp);
  fclose(fp);
  GenerateDiff("/tmp/1.dat", "/tmp/2.dat", "/tmp/diff.dat");
  ApplyPatch("/tmp/1.dat", "/tmp/diff.dat", "/tmp/3.dat", false);
  ApplyPatch("/tmp/2.dat", "/tmp/diff.dat", "/tmp/4.dat", true);
}

TEST(Basic, T2) {
  remove("/tmp/T2/index.db");
  remove("/tmp/T2/data.db");
  remove("/tmp/T2/meta.dat");
  {
    DiskMap<int, int> disk_map("index", "/tmp/T2/index.db", "data", "/tmp/T2/data.db");
    SnapShotManager snap_shot_manager;
    sjtu::vector<DataDriverBase *> drivers;
    drivers.push_back(&disk_map);
    snap_shot_manager.Connect(drivers);
    snap_shot_manager.SetMetaFile("/tmp/T2/meta.dat");
    for (int i = 0; i < 100000; i++) disk_map.Put(i, i);
    snap_shot_manager.CreateSnapShot("snap1");
  }
  {
    DiskMap<int, int> disk_map("index", "/tmp/T2/index.db", "data", "/tmp/T2/data.db");
    SnapShotManager snap_shot_manager;
    sjtu::vector<DataDriverBase *> drivers;
    drivers.push_back(&disk_map);
    snap_shot_manager.Connect(drivers);
    snap_shot_manager.SetMetaFile("/tmp/T2/meta.dat");
    for (int i = 0; i < 100; i += 10) disk_map.Put(i + 3, i);
    snap_shot_manager.CreateSnapShot("snap2");
    snap_shot_manager.CreateSnapShot("snap3");
  }
}