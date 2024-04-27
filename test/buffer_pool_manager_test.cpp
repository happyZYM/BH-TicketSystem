#include "bpt/buffer_pool_manager.h"
#include <gtest/gtest.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "MemoryRiver.hpp"
#include "MemoryRiverStd.hpp"
#include "bpt/bpt_page.hpp"
#include "bpt/config.h"
#include "bpt/disk_manager.h"
// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(Basic, BasicTest) {
  DiskManager disk_manager("/tmp/test.db");
  BufferPoolManager buffer_pool_manager(10, 3, &disk_manager);
}

TEST(BufferPoolManagerTest, BinaryDataTest) {
  const std::string db_name = "test.db";
  const size_t buffer_pool_size = 10;
  const size_t k = 5;

  std::random_device r;
  std::default_random_engine rng(r());
  std::uniform_int_distribution<char> uniform_dist(0);

  auto *disk_manager = new DiskManager(db_name);
  auto *bpm = new BufferPoolManager(buffer_pool_size, k, disk_manager);

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(&page_id_temp);

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  ASSERT_NE(nullptr, page0);
  EXPECT_EQ(1, page_id_temp);

  char random_binary_data[kPageSize];
  // Generate random binary data
  for (char &i : random_binary_data) {
    i = uniform_dist(rng);
  }

  // Insert terminal characters both in the middle and at end
  random_binary_data[kPageSize / 2] = '\0';
  random_binary_data[kPageSize - 1] = '\0';

  // Scenario: Once we have a page, we should be able to read and write content.
  std::memcpy(page0->GetData(), random_binary_data, kPageSize);
  EXPECT_EQ(0, std::memcmp(page0->GetData(), random_binary_data, kPageSize));

  // Scenario: We should be able to create new pages until we fill up the buffer pool.
  for (size_t i = 1; i < buffer_pool_size; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any new pages.
  for (size_t i = buffer_pool_size; i < buffer_pool_size * 2; ++i) {
    EXPECT_EQ(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: After unpinning pages {0, 1, 2, 3, 4} we should be able to create 5 new pages
  for (int i = 1; i <= 5; ++i) {
    EXPECT_EQ(true, bpm->UnpinPage(i, true));
    bpm->FlushPage(i);
  }
  for (int i = 1; i <= 5; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
    bpm->UnpinPage(page_id_temp, false);
  }
  // Scenario: We should be able to fetch the data we wrote a while ago.
  page0 = bpm->FetchPage(1);
  EXPECT_EQ(0, memcmp(page0->GetData(), random_binary_data, kPageSize));
  EXPECT_EQ(true, bpm->UnpinPage(1, true));

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->Close();
  remove("test.db");

  delete bpm;
  delete disk_manager;
}

// NOLINTNEXTLINE
TEST(BufferPoolManagerTest, SampleTest) {
  const std::string db_name = "test.db";
  const size_t buffer_pool_size = 10;
  const size_t k = 5;

  auto *disk_manager = new DiskManager(db_name);
  auto *bpm = new BufferPoolManager(buffer_pool_size, k, disk_manager);

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(&page_id_temp);

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  ASSERT_NE(nullptr, page0);
  EXPECT_EQ(1, page_id_temp);

  // Scenario: Once we have a page, we should be able to read and write content.
  snprintf(page0->GetData(), kPageSize, "Hello");
  EXPECT_EQ(0, strcmp(page0->GetData(), "Hello"));

  // Scenario: We should be able to create new pages until we fill up the buffer pool.
  for (size_t i = 2; i <= buffer_pool_size; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any new pages.
  for (size_t i = buffer_pool_size + 1; i <= buffer_pool_size * 2; ++i) {
    EXPECT_EQ(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: After unpinning pages {1, 2, 3, 4, 5} and pinning another 4 new pages,
  // there would still be one buffer page left for reading page 0.
  for (int i = 1; i <= 5; ++i) {
    EXPECT_EQ(true, bpm->UnpinPage(i, true));
  }
  for (int i = 1; i <= 4; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: We should be able to fetch the data we wrote a while ago.
  page0 = bpm->FetchPage(1);
  EXPECT_EQ(0, strcmp(page0->GetData(), "Hello"));

  // Scenario: If we unpin page 0 and then make a new page, all the buffer pages should
  // now be pinned. Fetching page 0 should fail.
  EXPECT_EQ(true, bpm->UnpinPage(1, true));
  EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  EXPECT_EQ(nullptr, bpm->FetchPage(1));

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->Close();
  remove("test.db");

  delete bpm;
  delete disk_manager;
}

TEST(StoreTest, Test1) {
  remove("/tmp/test.db");
  DiskManager *disk_manager_ptr = new DiskManager("/tmp/test.db");
  BufferPoolManager *buffer_pool_manager = new BufferPoolManager(10, 3, disk_manager_ptr);
  char *mem = buffer_pool_manager->RawDataMemory();
  uint32_t a = 0x1f2f3f4f;
  memcpy(mem, &a, sizeof(uint32_t));
  delete buffer_pool_manager;
  delete disk_manager_ptr;
  disk_manager_ptr = new DiskManager("/tmp/test.db");
  buffer_pool_manager = new BufferPoolManager(10, 3, disk_manager_ptr);
  mem = buffer_pool_manager->RawDataMemory();
  uint32_t b;
  memcpy(&b, mem, sizeof(uint32_t));
  EXPECT_EQ(a, b);
  delete buffer_pool_manager;
  delete disk_manager_ptr;
  disk_manager_ptr = new DiskManager("/tmp/test.db");
  buffer_pool_manager = new BufferPoolManager(10, 3, disk_manager_ptr);
  page_id_t page_id;
  auto basic_guard = buffer_pool_manager->NewPageGuarded(&page_id);
  typedef BPlusTreePage<unsigned long long> PageType;
  PageType c;
  c.data.p_n = 0x1f2f3f4f;
  c.data.key_count = 0x1f2a;
  c.data.page_status = 0x3e;
  c.data.p_data[17].first = 0x8f7f6f5f4f3f2f1f;
  c.filler[0] = 0x1f;
  *basic_guard.AsMut<PageType>() = c;
  basic_guard.Drop();
  auto read_guard = buffer_pool_manager->FetchPageRead(page_id);
  EXPECT_EQ(c.data.p_n, read_guard.As<PageType>()->data.p_n);
  EXPECT_EQ(0, memcmp(&c, read_guard.As<PageType>(), sizeof(PageType)));
  read_guard.Drop();
  delete buffer_pool_manager;
  delete disk_manager_ptr;
  disk_manager_ptr = new DiskManager("/tmp/test.db");
  buffer_pool_manager = new BufferPoolManager(10, 3, disk_manager_ptr);
  read_guard = buffer_pool_manager->FetchPageRead(page_id);
  EXPECT_EQ(c.data.p_n, read_guard.As<PageType>()->data.p_n);
  EXPECT_EQ(0, memcmp(&c, read_guard.As<PageType>(), sizeof(PageType)));
  read_guard.Drop();
  delete buffer_pool_manager;
  delete disk_manager_ptr;
}

TEST(MemoryRiver, T1) {
  typedef unsigned long long DataType;
  std::vector<DataType> record;
  std::vector<size_t> id_record;
  const int test_cnt = 30000;
  remove("/tmp/test2.db");
  {
    MemoryRiver<DataType> river;
    river.initialise("/tmp/test2.db");
    int x = 3;
    river.write_info(x, 1);
    DataType dat1 = 0x1f2f3f4f5f6f7f8f;
    for (int i = 0; i < test_cnt; i++) {
      DataType tmp = dat1 + i;
      size_t element_id = river.write(tmp);
      record.push_back(tmp);
      id_record.push_back(element_id);
    }
    for (int i = 0; i < test_cnt; i++) {
      DataType tmp;
      river.read(tmp, id_record[i]);
      EXPECT_EQ(record[i], tmp);
    }
    std::random_device r;
    std::default_random_engine rng(r());
    for (int i = 0; i < 1000; i++) {
      int t = rng() % record.size();
      river.Delete(id_record[t]);
      record.erase(record.begin() + t);
      id_record.erase(id_record.begin() + t);
    }
  }
  {
    MemoryRiver<DataType> river("/tmp/test2.db");
    int x;
    river.get_info(x, 1);
    EXPECT_EQ(3, x);
    for (int i = 0; i < test_cnt; i++) {
      DataType tmp;
      river.read(tmp, id_record[i]);
      EXPECT_EQ(record[i], tmp);
    }
  }
}

template <size_t length>
class FixLengthString {
 public:
  char data[length];
  FixLengthString &operator=(const FixLengthString &other) {
    memcpy(data, other.data, length);
    return *this;
  }
  bool operator==(const FixLengthString &other) const { return memcmp(data, other.data, length) == 0; }
};
TEST(MemoryRiver, T2) {
  spdlog::set_level(spdlog::level::err);
  auto logger_ptr = spdlog::stderr_color_mt("stderr_logger");
  const static size_t string_len = 120;
  typedef FixLengthString<string_len> DataType;
  std::deque<size_t> index_collection;
  std::unordered_map<size_t, std::pair<int, int>> index_track;
  size_t interal_id_tot = 0;
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  remove("T2.std");
  remove("T2.dat");
  const int kInfoLength = 100;
  {
    sol::MemoryRiver<DataType, kInfoLength> STD("/tmp/T2.std");
    MemoryRiver<DataType, kInfoLength> mr("/tmp/T2.dat");
    int total_opts = 1000000;
    while (total_opts-- > 0) {
      int opt = rnd() % 6;
      switch (opt) {
        case 0: {
          // get_info
          int idx = 1 + rnd() % kInfoLength;
          int std_ans, mr_ans;
          STD.get_info(std_ans, idx);
          mr.get_info(mr_ans, idx);
          EXPECT_EQ(std_ans, mr_ans);
          break;
        }
        case 1: {
          // write_info
          int idx = 1 + rnd() % kInfoLength;
          int val = rnd();
          STD.write_info(val, idx);
          mr.write_info(val, idx);
          break;
        }
        case 2: {
          // write
          interal_id_tot++;
          index_collection.push_back(interal_id_tot);
          DataType tmp;
          for (int i = 0; i < string_len; i++) tmp.data[i] = 'a' + rnd() % 26;
          tmp.data[string_len - 1] = '\0';
          index_track[interal_id_tot].first = STD.write(tmp);
          index_track[interal_id_tot].second = mr.write(tmp);
          logger_ptr->info("Write: {}", tmp.data);
          logger_ptr->info("internal id: {}", interal_id_tot);
          logger_ptr->info("index in STD: {}", index_track[interal_id_tot].first);
          logger_ptr->info("index in MR: {}", index_track[interal_id_tot].second);
          break;
        }
        case 3: {
          // update
          if (index_collection.empty()) goto nxt;
          size_t selected_internal_id = index_collection[rnd() % index_collection.size()];
          DataType tmp;
          for (int i = 0; i < string_len; i++) tmp.data[i] = 'a' + rnd() % 26;
          tmp.data[string_len - 1] = '\0';
          STD.update(tmp, index_track[selected_internal_id].first);
          mr.update(tmp, index_track[selected_internal_id].second);
          logger_ptr->info("Update: {}", tmp.data);
          logger_ptr->info("internal id: {}", selected_internal_id);
          logger_ptr->info("index in STD: {}", index_track[selected_internal_id].first);
          logger_ptr->info("index in MR: {}", index_track[selected_internal_id].second);
          break;
        }
        case 4: {
          // read
          if (index_collection.empty()) goto nxt;
          size_t selected_internal_id = index_collection[rnd() % index_collection.size()];
          DataType std_ans, mr_ans;
          STD.read(std_ans, index_track[selected_internal_id].first);
          mr.read(mr_ans, index_track[selected_internal_id].second);
          logger_ptr->info("Read: {}", selected_internal_id);
          logger_ptr->info("MR: read {} from {}", mr_ans.data, index_track[selected_internal_id].second);
          logger_ptr->info("STD: read {} from {}", std_ans.data, index_track[selected_internal_id].first);
          EXPECT_EQ(std_ans, mr_ans);
        }
        case 5: {
          // Delete
          if (index_collection.empty()) goto nxt;
          size_t selected_internal_id = index_collection[rnd() % index_collection.size()];
          logger_ptr->info("Delete: {}", selected_internal_id);
          logger_ptr->info("index in STD: {}", index_track[selected_internal_id].first);
          logger_ptr->info("index in MR: {}", index_track[selected_internal_id].second);
          STD.Delete(index_track[selected_internal_id].first);
          mr.Delete(index_track[selected_internal_id].second);
          index_collection.erase(std::find(index_collection.begin(), index_collection.end(), selected_internal_id));
          index_track.erase(selected_internal_id);
          break;
        }
      }
    nxt:;
    }
  }
}