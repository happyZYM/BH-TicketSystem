#ifndef CONFIG_H
#define CONFIG_H
#include <cstddef>
#include <cstdint>
extern const size_t kPageSize;
typedef uint32_t default_numeric_index_t;
typedef default_numeric_index_t page_id_t;
typedef default_numeric_index_t frame_id_t;
typedef default_numeric_index_t b_plus_tree_value_index_t;
extern const b_plus_tree_value_index_t kInvalidValueIndex;
typedef uint8_t page_status_t;
typedef uint16_t in_page_key_count_t;
typedef uint64_t bpt_size_t;
enum PageStatusType {
  LEAF = 1,
  INTERNAL = 2,
  ROOT = 4,
};
#endif