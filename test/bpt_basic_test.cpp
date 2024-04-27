#include <gtest/gtest.h>
#include <map>
#include "bpt/bpt.hpp"
#include "bpt/buffer_pool_manager.h"
#include "bpt/config.h"
#include "bpt/disk_manager.h"
namespace bpt_basic_test {
template <size_t length>
class FixLengthString {
 public:
  char data[length];
};
}  // namespace bpt_basic_test
TEST(BasicTest, Compile) {  // This Test only test the compile of the code
  // test for long long, int, char, long double
  BPlusTreePage<long long> page_long_long;
  static_assert(sizeof(page_long_long) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<int> page_int;
  static_assert(sizeof(page_int) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<char> page_char;
  static_assert(sizeof(page_char) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<long double> page_long_double;
  static_assert(sizeof(page_long_double) == 4096, "BPlusTreePage size mismatch");

  // test for FixLengthString with size = 5, 10, 15, 20, 25, 30, 35, 40;
  BPlusTreePage<bpt_basic_test::FixLengthString<5>> page_5;
  static_assert(sizeof(page_5) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<10>> page_10;
  static_assert(sizeof(page_10) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<15>> page_15;
  static_assert(sizeof(page_15) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<20>> page_20;
  static_assert(sizeof(page_20) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<25>> page_25;
  static_assert(sizeof(page_25) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<30>> page_30;
  static_assert(sizeof(page_30) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<35>> page_35;
  static_assert(sizeof(page_35) == 4096, "BPlusTreePage size mismatch");
  BPlusTreePage<bpt_basic_test::FixLengthString<40>> page_40;
  static_assert(sizeof(page_40) == 4096, "BPlusTreePage size mismatch");

  remove("/tmp/bpt1.db");
  DiskManager *dm = new DiskManager("/tmp/bpt1.db");
  BufferPoolManager *bpm = new BufferPoolManager(10, 3, dm);
  BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
  auto it = bpt.lower_bound(1);
  bpt.Flush();
  bpt.Get(1);
  it.SetValue(2);
  bpt.Put(1, 2);
  bpt.Remove(1);
  delete bpm;
  delete dm;
}