#ifndef BPT_PAGE_HPP
#define BPT_PAGE_HPP
#include <utility>
#include "bpt/config.h"
template <typename KeyType, size_t kPageSize = 4096>
struct ActualDataType {
  typedef std::pair<KeyType, default_numeric_index_t> value_type;
  page_id_t p_n;
  page_status_t page_status;  // root(2) / internal(1) / leaf(0)
  in_page_key_count_t key_count;
  const static size_t kMaxKeyCount =
      (kPageSize - sizeof(page_id_t) - sizeof(page_status_t) - sizeof(in_page_key_count_t)) / sizeof(value_type);
  value_type p_data[kMaxKeyCount];
  static_assert(kMaxKeyCount >= 2, "kMaxKeyCount must be greater than or equal to 2");
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