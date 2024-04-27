#ifndef BPT_HPP
#define BPT_HPP
#include <algorithm>
#include <shared_mutex>
#include <vector>
#include "bpt/bpt_page.hpp"
#include "bpt/buffer_pool_manager.h"
#include "bpt/config.h"
/**
 * @brief B+ Tree Indexer
 * @warning The KeyType must can be stored byte by byte. As this is only the indexer, the type of value is always
 * b_plus_tree_value_index_t.
 */
template <typename KeyType, typename KeyComparator>
class BPlusTreeIndexer {
  typedef BPlusTreePage<KeyType> PageType;
  typedef std::pair<KeyType, default_numeric_index_t> key_index_pair_t;
  typedef std::pair<KeyType, b_plus_tree_value_index_t> value_type;

 private:
  struct PositionSignType {
    std::vector<std::pair<BasicPageGuard, in_page_key_count_t>> path;
    bool is_end{false};
  };
  PositionSignType FindPosition(const KeyType &key) {  // Finish Design
    if (root_page_id == 0) {
      return PositionSignType{.is_end = true};
    }
    BasicPageGuard current_page_guard(bpm->FetchPageBasic(root_page_id));
    static auto comparer_for_key_index_pair = [](const key_index_pair_t &a, const KeyType &b) {
      return key_cmp(a.first, b);
    };
    in_page_key_count_t nxt = std::lower_bound(current_page_guard.As<PageType>()->data.p_data,
                                               current_page_guard.As<PageType>()->data.p_data +
                                                   current_page_guard.As<PageType>()->data.key_count,
                                               key, comparer_for_key_index_pair) -
                              current_page_guard.As<PageType>()->data.p_data;
    PositionSignType res;
    while (res.path.back().first.template As<PageType>()->data.page_status != 0) {
      default_numeric_index_t nxt_page_id;
      in_page_key_count_t internal_id = res.path.back().second;
      if (internal_id < res.path.back().first.template As<PageType>()->data.key_count)
        nxt_page_id = res.path.back().first.template As<PageType>()->data.p_data[internal_id].second;
      else
        nxt_page_id = res.path.back().first.template As<PageType>()->data.p_n;
      BasicPageGuard next_page_guard(bpm->FetchPageBasic(nxt_page_id));
      nxt =
          std::lower_bound(next_page_guard.As<PageType>()->data.p_data,
                           next_page_guard.As<PageType>()->data.p_data + next_page_guard.As<PageType>()->data.key_count,
                           key, comparer_for_key_index_pair) -
          next_page_guard.As<PageType>()->data.p_data;
      res.path.push_back(std::make_pair(std::move(next_page_guard), nxt));
    }
    if (nxt == res.path.back().first.template As<PageType>()->data.key_count) res.is_end = true;
    return res;
  }
  void InsertEntryAt(PositionSignType &pos, const KeyType &key, b_plus_tree_value_index_t value) {
    // TODO
  }
  void RemoveEntryAt(PositionSignType &pos) {
    // TODO
  }

 public:
  // note that for safety, the iterator is not copyable, and the const_iterator is not copyable
  class iterator {
    BPlusTreeIndexer *domain;
    size_t internal_offset;
    bool is_end;
    WritePageGuard guard;
    friend class BPlusTreeIndexer;

   public:
    const KeyType &GetKey() const { return guard.As<PageType>()->data.p_data[internal_offset].first; }
    const b_plus_tree_value_index_t &GetValue() { return guard.As<PageType>()->data.p_data[internal_offset].second; }
    bool operator==(iterator &that) {
      return domain == that.domain && guard.PageId() == that.guard.PageId() &&
             internal_offset == that.internal_offset && is_end == that.is_end;
    }
    void SetValue(b_plus_tree_value_index_t new_value) {
      guard.AsMut<PageType>()->data.p_data[internal_offset].second = new_value;
    }
    // only support ++it
    iterator &operator++() {
      if (is_end) return *this;
      internal_offset++;
      if (internal_offset == guard.As<PageType>()->data.key_count) {
        default_numeric_index_t nxt_page_id = guard.As<PageType>()->data.p_n;
        if (nxt_page_id == 0) {
          is_end = true;
          return *this;
        }
        guard = domain->bpm->FetchPageWrite(nxt_page_id);
        internal_offset = 0;
      }
      return *this;
    }
  };
  class const_iterator {
    BPlusTreeIndexer *domain;
    size_t internal_offset;
    bool is_end;
    ReadPageGuard guard;
    friend class BPlusTreeIndexer;

   public:
    const KeyType &GetKey() { return guard.As<PageType>()->data.p_data[internal_offset].first; }
    const b_plus_tree_value_index_t &GetValue() { return guard.As<PageType>()->data.p_data[internal_offset].second; }
    bool operator==(const_iterator &that) {
      return domain == that.domain && guard.PageId() == that.guard.PageId() &&
             internal_offset == that.internal_offset && is_end == that.is_end;
    }
    // only support ++it
    const_iterator &operator++() {
      if (is_end) return *this;
      internal_offset++;
      if (internal_offset == guard.As<PageType>()->data.key_count) {
        default_numeric_index_t nxt_page_id = guard.As<PageType>()->data.p_n;
        if (nxt_page_id == 0) {
          is_end = true;
          return *this;
        }
        guard = domain->bpm->FetchPageRead(nxt_page_id);
        internal_offset = 0;
      }
      return *this;
    }
  };
  BPlusTreeIndexer() = delete;
  BPlusTreeIndexer(const BPlusTreeIndexer &) = delete;
  BPlusTreeIndexer(BPlusTreeIndexer &&) = delete;
  BPlusTreeIndexer &operator=(const BPlusTreeIndexer &) = delete;
  BPlusTreeIndexer &operator=(BPlusTreeIndexer &&) = delete;
  BPlusTreeIndexer(BufferPoolManager *bpm_) {
    bpm = bpm_;
    raw_data_memory = bpm->RawDataMemory();
    memcpy(&root_page_id, raw_data_memory, sizeof(page_id_t));
    memcpy(&siz, raw_data_memory + sizeof(page_id_t), sizeof(bpt_size_t));
  }
  iterator end() { // Finish Design
    iterator res;
    res.domain = this;
    res.is_end = true;
    return res;
  }
  const_iterator end_const() { // Finish Design
    const_iterator res;
    res.domain = this;
    res.is_end = true;
    return res;
  }
  iterator lower_bound(const KeyType &key) {  // Finish Design
    std::shared_lock<std::shared_mutex> guard(latch);
    PositionSignType pos(std::move(FindPosition(key)));
    iterator res;
    res.domain = this;
    res.guard = bpm->FetchPageWrite(pos.path.back().first.PageId());
    res.is_end = pos.is_end;
    res.internal_offset = pos.path.back().second;
    return res;
  }
  const_iterator lower_bound_const(const KeyType &key) {  // Finish Design
    std::shared_lock<std::shared_mutex> guard(latch);
    PositionSignType pos(std::move(FindPosition(key)));
    const_iterator res;
    res.domain = this;
    res.guard = bpm->FetchPageRead(pos.path.back().first.PageId());
    res.is_end = pos.is_end;
    res.internal_offset = pos.path.back().second;
    return res;
  }
  b_plus_tree_value_index_t Get(const KeyType &key) { // Finish Design
    auto it = lower_bound_const(key);
    if (it == end_const()) return kInvalidValueIndex;
    if (key_cmp(key, it.GetKey())) return kInvalidValueIndex;
    return it.GetValue();
  }
  bool Put(const KeyType &key, b_plus_tree_value_index_t value) { // Finish Design
    PositionSignType pos(std::move(FindPosition(key)));
    if (!pos.is_end &&
        !key_cmp(key, pos.path.back().first.template As<PageType>()->data.p_data[pos.path.back().second].first)) {
      pos.path.back().first.template AsMut<PageType>()->data.p_data[pos.path.back().second].second = value;
      return false;
    }
    InsertEntryAt(pos, key, value);
    return true;
  }
  bool Remove(const KeyType &key) { // Finish Design
    PositionSignType pos(std::move(FindPosition(key)));
    if (pos.is_end) return false;
    if (key_cmp(key, pos.path.back().first.template As<PageType>()->data.p_data[pos.path.back().second].first))
      return false;
    RemoveEntryAt(pos);
    return true;
  }
  size_t Size() { return siz; }  // Finish Design
  void Flush() {                 // Finish Design
    memcpy(raw_data_memory, &root_page_id, sizeof(page_id_t));
    memcpy(raw_data_memory + sizeof(page_id_t), &siz, sizeof(bpt_size_t));
    bpm->FlushAllPages();
  }

 private:
  page_id_t root_page_id;  // stored in the first 4 (0-3) bytes of RawDatMemory, this directly operates on the buf
                           // maintained by DiskManager, BufferPoolManager only passes the pointer to it
  bpt_size_t siz;          // stored in the next 8 (4-11) bytes of RawDatMemory, this directly operates on the buf
                           // maintained by DiskManager, BufferPoolManager only passes the pointer to it
  static KeyComparator key_cmp;
  std::shared_mutex latch;
  BufferPoolManager *bpm;
  char *raw_data_memory;
};
template <typename KeyType, typename KeyComparator>
KeyComparator BPlusTreeIndexer<KeyType, KeyComparator>::key_cmp = KeyComparator();
#endif  // BPT_HPP