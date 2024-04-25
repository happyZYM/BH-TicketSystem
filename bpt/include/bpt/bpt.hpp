#ifndef BPT_HPP
#define BPT_HPP
#include <shared_mutex>
#include "bpt/bpt_page.hpp"
#include "bpt/buffer_pool_manager.h"
#include "bpt/config.h"
template <typename KeyType, typename KeyComparator>
class BPlusTreeIndexer {
 private:
  // TODO : insert ?
 public:
  typedef std::pair<KeyType, b_plus_tree_value_index_t> value_type;
  class iterator {
    BPlusTreeIndexer *domain;
    size_t internal_offset;
    bool is_end;
    WritePageGuard guard;
    const KeyType &GetKey() const {
      // TODO
    }
    const b_plus_tree_value_index_t &GetValue() const {
      // TODO
    }
  };
  class const_iterator {
    BPlusTreeIndexer *domain;
    size_t internal_offset;
    bool is_end;
    ReadPageGuard guard;
    const KeyType &GetKey() const {
      // TODO
    }
    const b_plus_tree_value_index_t &GetValue() const {
      // TODO
    }
  };
  BPlusTreeIndexer(const BPlusTreeIndexer &) = delete;
  BPlusTreeIndexer(BPlusTreeIndexer &&) = delete;
  BPlusTreeIndexer &operator=(const BPlusTreeIndexer &) = delete;
  BPlusTreeIndexer &operator=(BPlusTreeIndexer &&) = delete;
  iterator end() {
    // TODO
  }
  iterator lower_bound(const KeyType &key) {
    std::shared_lock<std::shared_mutex> guard(latch);
    // TODO
  }
  const_iterator lower_bound_const(const KeyType &key) {
    std::shared_lock<std::shared_mutex> guard(latch);
    // TODO
  }
  bool Set(const iterator &iter, b_plus_tree_value_index_t value) {
    std::unique_lock<std::shared_mutex> guard(latch);
    // TODO
  }
  bool Erase(const iterator &iter) {
    std::unique_lock<std::shared_mutex> guard(latch);
    // TODO
  }
  b_plus_tree_value_index_t Get(const KeyType &key) {
    auto it = lower_bound_const(key);
    if (it == end()) return kInvalidValueIndex;
    if (key_cmp(key, it.GetKey())) return kInvalidValueIndex;
    return it->second;
  }
  bool Put(const KeyType &key, b_plus_tree_value_index_t value) {
    auto it = lower_bound(key);
    if (it != end() && !key_cmp(key, it.GetKey())) {
      Set(it, value);
      return false;
    }
    // TODO Insert it
    return true;
  }
  bool Remove(const KeyType &key) {
    auto it = lower_bound(key);
    if (it == end()) return false;
    if (key_cmp(key, it.GetKey())) return false;
    Erase(it);
    return true;
  }
  size_t Size() { return siz; }

 private:
  page_id_t root_page_id;
  size_t siz;
  static KeyComparator key_cmp;
  std::shared_mutex latch;
};
template <typename KeyType, typename KeyComparator>
KeyComparator BPlusTreeIndexer<KeyType, KeyComparator>::key_cmp = KeyComparator();
#endif  // BPT_HPP