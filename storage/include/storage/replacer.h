#ifndef REPLACER_H
#define REPLACER_H
#include <cstddef>
#include <mutex>
#include "storage/config.h"
class LRUKReplacer {
 public:
  LRUKReplacer() = delete;
  LRUKReplacer(const LRUKReplacer &) = delete;
  LRUKReplacer(LRUKReplacer &&) = delete;
  explicit LRUKReplacer(size_t max_frame_count, size_t k_value);
  ~LRUKReplacer();
  bool TryEvictLeastImportant(frame_id_t &frame_id);
  void RecordAccess(frame_id_t frame_id);
  void SetEvictable(frame_id_t frame_id, bool evitable);
  bool TryEvictExactFrame(frame_id_t frame_id);
  LRUKReplacer &operator=(const LRUKReplacer &) = delete;
  LRUKReplacer &operator=(LRUKReplacer &&) = delete;
  size_t GetCurrentEvitableCount();

 private:
  struct LRUChainNodeType {  // for not has k visit
    frame_id_t frame_id;
    LRUChainNodeType *prev, *next;
    LRUChainNodeType() = delete;
    explicit LRUChainNodeType(frame_id_t frame_id, LRUChainNodeType *prev, LRUChainNodeType *next)
        : frame_id(frame_id), prev(prev), next(next) {}
  };
  struct MainChainNodeType {  // for has k visit
    frame_id_t frame_id;
    size_t time_stamp;
    MainChainNodeType *prev, *next;
    MainChainNodeType *next_self_record;
    MainChainNodeType() = delete;
    explicit MainChainNodeType(frame_id_t frame_id, size_t time_stamp, MainChainNodeType *prev, MainChainNodeType *next,
                               MainChainNodeType *next_self_record)
        : frame_id(frame_id), time_stamp(time_stamp), prev(prev), next(next), next_self_record(next_self_record) {}
  };
  template <typename ListNodeType>
  inline void RemoveFromList(ListNodeType *node) {
    if (node->prev != nullptr) {
      node->prev->next = node->next;
    }
    if (node->next != nullptr) {
      node->next->prev = node->prev;
    }
  }
  template <typename ListNodeType>
  inline void InsertAt(ListNodeType *node, ListNodeType *prev, ListNodeType *next) {
    node->prev = prev;
    node->next = next;
    if (prev != nullptr) prev->next = node;
    if (next != nullptr) next->prev = node;
  }
  struct LRUKRecord {
    LRUKRecord() = default;
    bool evitable;
    size_t visit_count;
    bool active;
    MainChainNodeType *head_node_in_main_chain, *tail_node_in_main_chain;
    LRUChainNodeType *node_in_LRU_chain;
  };
  void RemoveWholeFrameFromLRUKChain(MainChainNodeType *first_occurrence_ptr);  // remove and delete nodes
  MainChainNodeType *AddRecordToMainChain(frame_id_t frame_it, size_t time_stamp,
                                          MainChainNodeType *last_node_in_main_chain);
  LRUChainNodeType *LRU_chain_head_guard, *LRU_chain_tail_guard;
  MainChainNodeType *LRUK_chain_head_guard, *LRUK_chain_tail_guard;
  size_t current_timestamp_{0};
  size_t current_evitable_count_{0};
  size_t max_frame_count;
  size_t k_value;
#ifdef ENABLE_ADVANCED_FEATURE
  std::mutex latch;
#endif
  LRUKRecord *hash_for_record;
};
#endif