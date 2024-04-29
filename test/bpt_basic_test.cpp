#include <gtest/gtest.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <map>
#include <random>
#include "bpt/bpt.hpp"
#include "bpt/buffer_pool_manager.h"
#include "bpt/config.h"
#include "bpt/disk_manager.h"
namespace bpt_basic_test {
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
}  // namespace bpt_basic_test
TEST(BasicTest, Compile) {  // This Test only test the compile of the code
  return;
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

TEST(BasicTest, Put_and_Get) {
  remove("/tmp/bpt2.db");
  DiskManager *dm = new DiskManager("/tmp/bpt2.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    bpt.Put(1, 2);
    ASSERT_EQ(bpt.Get(1), 2);
    bpt.Put(2, 5);
    ASSERT_EQ(bpt.Get(2), 5);
    bpt.Put(3, 7);
    ASSERT_EQ(bpt.Get(3), 7);
    bpt.Put(4, 11);
    ASSERT_EQ(bpt.Get(4), 11);
    bpt.Put(2, 15);
    ASSERT_EQ(bpt.Get(2), 15);
    ASSERT_EQ(bpt.Get(3), 7);
    ASSERT_EQ(bpt.Get(1), 2);
    ASSERT_EQ(bpt.Get(4), 11);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt2.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    ASSERT_EQ(bpt.Get(2), 15);
    ASSERT_EQ(bpt.Get(3), 7);
    ASSERT_EQ(bpt.Get(1), 2);
    ASSERT_EQ(bpt.Get(4), 11);
  }
  delete bpm;
  delete dm;
}

TEST(BasicTest, Put_Get_Remove) {
  remove("/tmp/bpt3.db");
  DiskManager *dm = new DiskManager("/tmp/bpt3.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    bpt.Put(1, 2);
    ASSERT_EQ(bpt.Get(1), 2);
    bpt.Put(2, 5);
    ASSERT_EQ(bpt.Get(2), 5);
    bpt.Put(3, 7);
    ASSERT_EQ(bpt.Get(3), 7);
    bpt.Put(4, 11);
    ASSERT_EQ(bpt.Get(4), 11);
    bpt.Put(2, 15);
    ASSERT_EQ(bpt.Get(2), 15);
    ASSERT_EQ(bpt.Get(3), 7);
    ASSERT_EQ(bpt.Get(1), 2);
    bpt.Put(9, 11);
    ASSERT_EQ(bpt.Get(4), 11);
    bpt.Remove(2);
    bpt.Remove(2);
    ASSERT_EQ(bpt.Get(2), kInvalidValueIndex);
    bpt.Remove(3);
    ASSERT_EQ(bpt.Get(3), kInvalidValueIndex);
    bpt.Remove(1);
    ASSERT_EQ(bpt.Get(1), kInvalidValueIndex);
    bpt.Remove(4);
    bpt.Remove(4);
    ASSERT_EQ(bpt.Get(4), kInvalidValueIndex);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt3.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    ASSERT_EQ(bpt.Get(2), kInvalidValueIndex);
    ASSERT_EQ(bpt.Get(3), kInvalidValueIndex);
    ASSERT_EQ(bpt.Get(1), kInvalidValueIndex);
    ASSERT_EQ(bpt.Get(4), kInvalidValueIndex);
    ASSERT_EQ(bpt.Get(9), 11);
  }
  delete bpm;
  delete dm;
}

TEST(BasicTest, Split_in_Put_Simple_1) {
  remove("/tmp/bpt4.db");
  DiskManager *dm = new DiskManager("/tmp/bpt4.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    for (int i = 1; i <= 383; i++) {
      bpt.Put(i, i + 3);
      ASSERT_EQ(bpt.Get(i), i + 3);
    }
    for (int i = 1; i <= 383; i++) {
      ASSERT_EQ(bpt.Get(i), i + 3);
    }
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt4.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    for (int i = 1; i <= 383; i++) {
      ASSERT_EQ(bpt.Get(i), i + 3);
    }
  }
  delete bpm;
  delete dm;
}

TEST(BasicTest, Split_in_Put_Simple_2) {
  std::vector<int> keys;
  const int kNumberOfKeys = 383;
  for (int i = 1; i <= kNumberOfKeys; i++) keys.push_back(i);
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  std::shuffle(keys.begin(), keys.end(), rnd);
  remove("/tmp/bpt5.db");
  DiskManager *dm = new DiskManager("/tmp/bpt5.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    for (int i = 1; i <= kNumberOfKeys; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    for (int i = 1; i <= kNumberOfKeys; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt5.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    for (int i = 1; i <= kNumberOfKeys; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
  }
  delete bpm;
  delete dm;
}

TEST(BasicTest, Split_in_Put_Simple_3) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 16;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  remove("/tmp/bpt5.db");
  DiskManager *dm = new DiskManager("/tmp/bpt5.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  std::vector<KeyType> keys;
  const int ops = 307;
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[15] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt5.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_1) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 1360 - 4;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  remove("/tmp/bpt6.db");
  DiskManager *dm = new DiskManager("/tmp/bpt6.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  std::vector<KeyType> keys;
  const int ops = 5;
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[15] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt6.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_2) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 2030;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  remove("/tmp/bpt7.db");
  DiskManager *dm = new DiskManager("/tmp/bpt7.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  std::vector<KeyType> keys;
  const int ops = 4;
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[15] = '\0';
    keys.push_back(key);
  }
  sort(keys.begin(), keys.end());
  // std::shuffle(keys.begin(), keys.end(), rnd);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt7.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_3) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 800;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt8.db";
  std::vector<KeyType> keys;
  const int ops = 1000;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_4) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 800;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt9.db";
  std::vector<KeyType> keys;
  const int ops = 1000;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  sort(keys.begin(), keys.end());
  // std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_5) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 800;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt10.db";
  std::vector<KeyType> keys;
  const int ops = 15 + rnd() % 20;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_6) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 1000;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt11.db";
  std::vector<KeyType> keys;
  const int ops = 15 + rnd() % 20;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_7) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 2000;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt12.db";
  std::vector<KeyType> keys;
  const int ops = 15 + rnd() % 20;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_8) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 1300;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt13.db";
  std::vector<KeyType> keys;
  const int ops = 15 + rnd() % 20;
  remove(db_file_name.c_str());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  for (int i = 1; i <= ops; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[str_len - 1] = '\0';
    keys.push_back(key);
  }
  // sort(keys.begin(), keys.end());
  std::shuffle(keys.begin(), keys.end(), rnd);
  // for (int i = 1; i <= ops; i++) fprintf(stderr, "key[%d]=%s\n", i - 1, keys[i - 1].data);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    for (int i = 1; i <= ops; i++) {
      bpt.Put(keys[i - 1], i + 3);
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
      ASSERT_EQ(bpt.Size(), i);
    }
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager(db_file_name.c_str());
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), ops);
    for (int i = 1; i <= ops; i++) {
      ASSERT_EQ(bpt.Get(keys[i - 1]), i + 3);
    }
    ASSERT_EQ(bpt.Size(), ops);
  }
  delete bpm;
  delete dm;
}

TEST(HarderTest, Split_in_Put_Harder_9) {
  std::vector<std::pair<int, int>> entries;
  const int kNumberOfKeys = 100000;
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(1);
  for (int i = 0; i < kNumberOfKeys; i++) {
    entries.push_back({i + 3, rnd()});
  }
  std::shuffle(entries.begin(), entries.end(), rnd);
  remove("/tmp/bpt14.db");
  DiskManager *dm = new DiskManager("/tmp/bpt14.db");
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    for (int i = 0; i < kNumberOfKeys; i++) {
      bpt.Put(entries[i].first, entries[i].second);
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
      ASSERT_EQ(bpt.Size(), i + 1);
    }
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
    std::shuffle(entries.begin(), entries.end(), rnd);
    for (int i = 0; i < kNumberOfKeys; i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt14.db");
  bpm = new BufferPoolManager(20, 3, dm);
  std::shuffle(entries.begin(), entries.end(), rnd);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
    for (int i = 0; i < kNumberOfKeys; i++) {
      ASSERT_EQ(bpt.Get(entries[i].first), entries[i].second);
    }
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
  }
  delete bpm;
  delete dm;
  dm = new DiskManager("/tmp/bpt14.db");
  bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<long long, std::less<long long>> bpt(bpm);
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
    sort(entries.begin(), entries.end());
    auto it_std = entries.begin();
    auto it_bpt = bpt.lower_bound_const(entries[0].first);
    for (int i = 0; i < kNumberOfKeys; i++) {
      ASSERT_EQ(it_bpt.GetKey(), it_std->first);
      ASSERT_EQ(it_bpt.GetValue(), it_std->second);
      ++it_bpt;
      it_std++;
    }
    ASSERT_TRUE(it_bpt == bpt.end_const());
    ASSERT_EQ(bpt.Size(), kNumberOfKeys);
  }
  delete bpm;
  delete dm;
}

TEST(RemoveTest, RM_1) {
  const unsigned int RndSeed = testing::GTEST_FLAG(random_seed);
  std::mt19937 rnd(RndSeed);
  const int str_len = 800;
  typedef bpt_basic_test::FixLengthString<str_len> KeyType;
  // fprintf(stderr, "sizeof(std::pair<KeyType, default_numeric_index_t>)=%lu\n",
  // sizeof(std::pair<KeyType, default_numeric_index_t>));
  const std::string db_file_name = "/tmp/bpt15.db";
  remove(db_file_name.c_str());
  std::vector<KeyType> keys;
  const int max_keys = 10;
  for (int i = 1; i <= max_keys; i++) {
    KeyType key;
    for (size_t j = 0; j < str_len; j++) key.data[j] = 'a' + rnd() % 26;
    key.data[6] = '\0';
    keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());
  DiskManager *dm = new DiskManager(db_file_name.c_str());
  BufferPoolManager *bpm = new BufferPoolManager(20, 3, dm);
  {
    BPlusTreeIndexer<KeyType, std::less<KeyType>> bpt(bpm);
    bpt.Put(keys[1], 4);
    bpt.Put(keys[0], 3);
    bpt.Put(keys[2], 5);
    bpt.Put(keys[3], 6);
    bpt.Put(keys[4], 7);
    bpt.Remove(keys[1]);
    ASSERT_EQ(bpt.Size(), 4);
    auto it = bpt.lower_bound_const(keys[0]);
    ASSERT_EQ(it.GetKey(), keys[0]);
    ASSERT_EQ(it.GetValue(), 3);
    ++it;
    ASSERT_EQ(it.GetKey(), keys[2]);
    ASSERT_EQ(it.GetValue(), 5);
    ++it;
    ASSERT_EQ(it.GetKey(), keys[3]);
    ASSERT_EQ(it.GetValue(), 6);
    ++it;
    ASSERT_EQ(it.GetKey(), keys[4]);
    ASSERT_EQ(it.GetValue(), 7);
    ++it;
    ASSERT_TRUE(it == bpt.end_const());
  }
  delete bpm;
  delete dm;
}