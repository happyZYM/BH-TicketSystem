#include <gtest/gtest.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <map>
#include <random>
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/config.h"
#include "storage/disk_manager.h"
namespace bpt_advanced_test {
template <size_t length>
class FixLengthString {
 public:
  char data[length];
  bool operator<(const FixLengthString<length> &that) const {
    for (size_t i = 0; i < length; i++) {
      if (data[i] < that.data[i]) return true;
      if (data[i] > that.data[i]) return false;
    }
    return false;
  }
  bool operator==(const FixLengthString<length> &that) const {
    for (size_t i = 0; i < length; i++) {
      if (data[i] != that.data[i]) return false;
    }
    return true;
  }
};
}  // namespace bpt_advanced_test
TEST(STRING, huge_size_1) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 10;
  typedef bpt_advanced_test::FixLengthString<str_len> KeyType;
  fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
          sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt17.db";
  remove(db_file_name.c_str());
  std::vector<std::pair<KeyType, int>> entries;
  const int max_keys = 1000;
  const int keys_num_to_remove = 990;
  for (int i = 1; i <= max_keys; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    entries.push_back(std::make_pair(key, i));
  }
  // std::sort(entries.begin(), entries.end());
  std::shuffle(entries.begin(), entries.end(), rnd);
  fprintf(stderr, "The entries are:\n");
  for (int i = 0; i < entries.size(); i++) {
    fprintf(stderr, "key[%d]=%s value[%d]=%d\n", i, entries[i].first.data, i, entries[i].second);
  }
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= max_keys; i++) {
      bpt.Put(entries[i - 1].first, entries[i - 1].second);
    }
    for (int i = 1; i <= keys_num_to_remove; i++) {
      // {
      //   // checking iteration
      //   auto it_std = entries.begin();
      //   auto it_bpt = bpt.lower_bound_const(entries[0].first);
      //   for (int i = 0; i < entries.size(); i++) {
      //     fprintf(stderr, "i=%d checking key[%d]=%s value[%d]=%d\n", i, i, it_std->first.data, i, it_std->second);
      //     ASSERT_TRUE(!(it_bpt == bpt.end_const()));
      //     ASSERT_EQ(it_bpt.GetKey(), it_std->first);
      //     ASSERT_EQ(it_bpt.GetValue(), it_std->second);
      //     ++it_bpt;
      //     it_std++;
      //   }
      //   ASSERT_TRUE(it_bpt == bpt.end_const());
      //   ASSERT_EQ(bpt.Size(), entries.size());
      // }
      int id = rnd() % entries.size();
      fprintf(stderr, "removing key[%d]=%s value[%d]=%d\n", id, entries[id].first.data, id, entries[id].second);
      bpt.Remove(entries[id].first);
      entries.erase(entries.begin() + id);
      ASSERT_EQ(bpt.Size(), entries.size());
      for (int j = 0; j < entries.size(); j++) {
        ASSERT_EQ(bpt.Get(entries[j].first), entries[j].second);
      }
      {
        // checking iteration
        std::sort(entries.begin(), entries.end());
        auto it_std = entries.begin();
        auto it_bpt = bpt.lower_bound_const(entries[0].first);
        for (int i = 0; i < entries.size(); i++) {
          fprintf(stderr, "i=%d checking key[%d]=%s value[%d]=%d\n", i, i, it_std->first.data, i, it_std->second);
          ASSERT_TRUE(!(it_bpt == bpt.end_const()));
          ASSERT_EQ(it_bpt.GetKey(), it_std->first);
          ASSERT_EQ(it_bpt.GetValue(), it_std->second);
          ++it_bpt;
          it_std++;
        }
        ASSERT_TRUE(it_bpt == bpt.end_const());
        ASSERT_EQ(bpt.Size(), entries.size());
      }
    }
    ASSERT_EQ(bpt.Size(), max_keys - keys_num_to_remove);
    for (int i = 0; i < entries.size(); i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 0; i < entries.size(); i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), entries.size());
    for (int i = 0; i < entries.size(); i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
    sort(entries.begin(), entries.end());
    for (int i = 0; i < entries.size(); i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
    auto it_std = entries.begin();
    auto it_bpt = bpt.lower_bound_const(entries[0].first);
    for (int i = 0; i < entries.size(); i++) {
      fprintf(stderr, "i=%d checking key[%d]=%s value[%d]=%d\n", i, i, it_std->first.data, i, it_std->second);
      ASSERT_TRUE(!(it_bpt == bpt.end_const()));
      ASSERT_EQ(it_bpt.GetKey(), it_std->first);
      ASSERT_EQ(it_bpt.GetValue(), it_std->second);
      ++it_bpt;
      it_std++;
    }
    ASSERT_TRUE(it_bpt == bpt.end_const());
    ASSERT_EQ(bpt.Size(), entries.size());
  }
  delete bpm;
  delete dm;
}

TEST(LONGLONG, huge_size_1) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  typedef long long KeyType;
  const std::string db_file_name = "/tmp/bpt18.db";
  remove(db_file_name.c_str());
  std::map<KeyType, int> entries;
  const int max_keys = 10000;
  const int keys_num_to_remove = 9900;
  for (int i = 1; i <= max_keys; i++) {
    KeyType key = rnd();
    entries[key] = i;
  }
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (auto &entry : entries) {
      bpt.Put(entry.first, entry.second);
    }
    for (int i = 1; i <= keys_num_to_remove; i++) {
      if (rnd() % 2 == 0) {
        int id = rnd() % entries.size();
        auto it = entries.begin();
        for (int j = 0; j < id; j++) it++;
        fprintf(stderr, "removing key=%lld value=%d\n", it->first, it->second);
        bpt.Remove(it->first);
        entries.erase(it);
      } else {
        // Put
        KeyType key = rnd();
        int value = rnd();
        fprintf(stderr, "inserting key=%lld value=%d\n", key, value);
        bpt.Put(key, value);
        entries[key] = value;
      }
      ASSERT_EQ(bpt.Size(), entries.size());
      for (auto &entry : entries) {
        ASSERT_EQ(bpt.Get(entry.first), entry.second);
      }
      {
        // checking iteration
        auto it_std = entries.begin();
        auto it_bpt = bpt.lower_bound_const(entries.begin()->first);
        for (int i = 0; i < entries.size(); i++) {
          fprintf(stderr, "i=%d checking key=%lld value=%d\n", i, it_std->first, it_std->second);
          ASSERT_TRUE(!(it_bpt == bpt.end_const()));
          ASSERT_EQ(it_bpt.GetKey(), it_std->first);
          ASSERT_EQ(it_bpt.GetValue(), it_std->second);
          ++it_bpt;
          it_std++;
        }
        ASSERT_TRUE(it_bpt == bpt.end_const());
        ASSERT_EQ(bpt.Size(), entries.size());
      }
    }
    ASSERT_EQ(bpt.Size(), entries.size());
    for (auto &entry : entries) {
      ASSERT_EQ(bpt.Get(entry.first), entry.second);
    }
  }
}