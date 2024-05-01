#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include "storage/buffer_pool_manager.h"
#include "storage/config.h"

TEST(PageGuardTest, SampleTest) {
  const std::string db_name = "/tmp/test.db";
  const size_t buffer_pool_size = 5;
  const size_t k = 2;

  auto disk_manager = std::make_shared<DiskManager>(db_name);
  auto bpm = std::make_shared<BufferPoolManager>(buffer_pool_size, k, disk_manager.get());

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(&page_id_temp);

  auto guarded_page = BasicPageGuard(bpm.get(), page0);

  EXPECT_EQ(page0->GetData(), guarded_page.GetData());
  EXPECT_EQ(page0->GetPageId(), guarded_page.PageId());
  EXPECT_EQ(1, page0->GetPinCount());

  guarded_page.Drop();

  EXPECT_EQ(0, page0->GetPinCount());

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->Close();
  remove(db_name.c_str());
}
