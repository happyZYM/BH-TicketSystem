#ifndef CONFIG_H
#define CONFIG_H
#include <cstddef>
extern const size_t kPageSize;
typedef unsigned int default_numeric_index_t;
typedef default_numeric_index_t page_id_t;
typedef default_numeric_index_t block_id_t;
typedef default_numeric_index_t frame_id_t;
typedef default_numeric_index_t b_plus_tree_value_index_t;
extern const b_plus_tree_value_index_t kInvalidValueIndex;
#endif