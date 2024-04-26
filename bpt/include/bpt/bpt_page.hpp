#ifndef BPT_PAGE_HPP
#define BPT_PAGE_HPP
#include <utility>
#include "bpt/config.h"
template <typename KeyType, size_t kPageSize = 4096>
struct ActualDataType {
  typedef std::pair<KeyType, default_numeric_index_t> value_type;
  page_id_t p_n;
  page_id_t p_parent;
  uint8_t is_leaf;
  uint16_t key_count;
  const static size_t kMaxKeyCount =
      (kPageSize - sizeof(page_id_t) * 2 - sizeof(uint8_t) - sizeof(uint16_t)) / sizeof(value_type);
  value_type p_data[kMaxKeyCount];
};
template <typename KeyType, size_t kPageSize = 4096>
union BPlusTreePage {
  inline BPlusTreePage() {}
  inline BPlusTreePage &operator=(const BPlusTreePage &that) {
    memcpy(this, &that, sizeof(BPlusTreePage));
    return *this;
  }
  ActualDataType<KeyType, kPageSize> data;
  char filler[kPageSize];
};
#endif  // BPT_PAGE_H