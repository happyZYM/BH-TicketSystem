#include "bpt/buffer_pool_manager.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "MemoryRiver.hpp"
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
  c.data.is_leaf = 0x3e;
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
  remove("/tmp/test2.db");
  typedef unsigned long long DataType;
  MemoryRiver<DataType> river;
  river.initialise("/tmp/test2.db");
  int x = 3;
  river.write_info(x, 1);
  DataType dat1 = 0x1f2f3f4f5f6f7f8f;
  std::vector<DataType> record;
  std::vector<size_t> id_record;
  int test_cnt = 3;
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
}