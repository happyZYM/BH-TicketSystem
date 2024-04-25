#include "bpt/replacer.h"
#include <gtest/gtest.h>
#include "bpt/config.h"
// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(BasicTest, Basic1) {
  LRUKReplacer replacer(6, 3);
  replacer.RecordAccess(0);
  replacer.RecordAccess(0);
  replacer.RecordAccess(0);
  frame_id_t frame_id;
  ASSERT_EQ(replacer.TryEvictLeastImportant(frame_id), false);
  replacer.RecordAccess(1);
  replacer.SetEvictable(0, true);
  replacer.SetEvictable(1, true);
  ASSERT_EQ(replacer.TryEvictLeastImportant(frame_id), true);
  ASSERT_EQ(frame_id, 1);
}

TEST(BasicTest, CopiedFromBustubProject) {
  LRUKReplacer lru_replacer(7, 2);

  // Scenario: add six elements to the replacer. We have [1,2,3,4,5]. Frame 6 is non-evictable.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(6);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  lru_replacer.SetEvictable(6, false);
  ASSERT_EQ(5, lru_replacer.GetCurrentEvitableCount());

  // Scenario: Insert access history for frame 1. Now frame 1 has two access histories.
  // All other frames have max backward k-dist. The order of eviction is [2,3,4,5,1].
  lru_replacer.RecordAccess(1);

  // Scenario: Evict three pages from the replacer. Elements with max k-distance should be popped
  // first based on LRU.
  frame_id_t value;
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(2, value);
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(3, value);
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(4, value);
  ASSERT_EQ(2, lru_replacer.GetCurrentEvitableCount());

  // Scenario: Now replacer has frames [5,1].
  // Insert new frames 3, 4, and update access history for 5. We should end with [3,1,5,4]
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  ASSERT_EQ(4, lru_replacer.GetCurrentEvitableCount());

  // Scenario: continue looking for victims. We expect 3 to be evicted next.
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(3, value);
  ASSERT_EQ(3, lru_replacer.GetCurrentEvitableCount());

  // Set 6 to be evictable. 6 Should be evicted next since it has max backward k-dist.
  lru_replacer.SetEvictable(6, true);
  ASSERT_EQ(4, lru_replacer.GetCurrentEvitableCount());
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(6, value);
  ASSERT_EQ(3, lru_replacer.GetCurrentEvitableCount());

  // Now we have [1,5,4]. Continue looking for victims.
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(2, lru_replacer.GetCurrentEvitableCount());
  ASSERT_EQ(true, lru_replacer.TryEvictLeastImportant(value));
  ASSERT_EQ(5, value);
  ASSERT_EQ(1, lru_replacer.GetCurrentEvitableCount());

  // Update access history for 1. Now we have [4,1]. Next victim is 4.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(2, lru_replacer.GetCurrentEvitableCount());
  ASSERT_EQ(true, lru_replacer.TryEvictLeastImportant(value));
  ASSERT_EQ(value, 4);

  ASSERT_EQ(1, lru_replacer.GetCurrentEvitableCount());
  lru_replacer.TryEvictLeastImportant(value);
  ASSERT_EQ(value, 1);
  ASSERT_EQ(0, lru_replacer.GetCurrentEvitableCount());

  // This operation should not modify size
  ASSERT_EQ(false, lru_replacer.TryEvictLeastImportant(value));
  ASSERT_EQ(0, lru_replacer.GetCurrentEvitableCount());
}