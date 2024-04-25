#ifndef BPT_PAGE_HPP
#define BPT_PAGE_HPP
#include <utility>
#include "bpt/config.h"
#pragma pack(push, 1)
template <typename KeyType, size_t kPageSize = 4096>
class BPlusTreePage {
  typedef std::pair<KeyType, b_plus_tree_value_index_t> value_type;
  page_id_t p_n;
  page_id_t p_parent;
  uint8_t is_leaf;
  uint16_t key_count;
  const static size_t kMaxKeyCount =
      (kPageSize - sizeof(page_id_t) * 2 - sizeof(uint8_t) - sizeof(uint16_t)) / sizeof(value_type);
  value_type p_data[kMaxKeyCount];
  char filler[kPageSize - sizeof(page_id_t) * 2 - sizeof(uint8_t) - sizeof(uint16_t) -
              sizeof(value_type) * kMaxKeyCount];
};
#pragma pack(pop)
#endif  // BPT_PAGE_H