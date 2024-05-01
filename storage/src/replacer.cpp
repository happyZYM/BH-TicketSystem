#include "storage/replacer.h"
#include <cstddef>
LRUKReplacer::LRUKReplacer(size_t max_frame_count, size_t k_value)
    : max_frame_count(max_frame_count), k_value(k_value) {
  hash_for_record = new LRUKRecord[max_frame_count];
  for (size_t i = 0; i < max_frame_count; i++) {
    hash_for_record[i].active = false;
  }
  LRU_chain_head_guard = new LRUChainNodeType(-1, nullptr, nullptr);
  LRU_chain_tail_guard = new LRUChainNodeType(-1, nullptr, nullptr);
  LRU_chain_head_guard->next = LRU_chain_tail_guard;
  LRU_chain_tail_guard->prev = LRU_chain_head_guard;
  LRUK_chain_head_guard = new MainChainNodeType(-1, 0, nullptr, nullptr, nullptr);
  LRUK_chain_tail_guard = new MainChainNodeType(-1, 0, nullptr, nullptr, nullptr);
  LRUK_chain_head_guard->next = LRUK_chain_tail_guard;
  LRUK_chain_tail_guard->prev = LRUK_chain_head_guard;
}

LRUKReplacer::~LRUKReplacer() {
  delete[] hash_for_record;
  LRUChainNodeType *ptr = LRU_chain_head_guard->next;
  while (ptr != LRU_chain_tail_guard) {
    LRUChainNodeType *tmp = ptr;
    ptr = ptr->next;
    delete tmp;
  }
  MainChainNodeType *ptr2 = LRUK_chain_head_guard->next;
  while (ptr2 != LRUK_chain_tail_guard) {
    MainChainNodeType *tmp = ptr2;
    ptr2 = ptr2->next;
    delete tmp;
  }
  delete LRU_chain_head_guard;
  delete LRU_chain_tail_guard;
  delete LRUK_chain_head_guard;
  delete LRUK_chain_tail_guard;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool evitable) {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (!hash_for_record[frame_id].active) {
    return;
  }
  if (hash_for_record[frame_id].evitable == evitable) {
    return;
  }
  hash_for_record[frame_id].evitable = evitable;
  if (evitable) {
    current_evitable_count_++;
  } else {
    current_evitable_count_--;
  }
}

void LRUKReplacer::RemoveWholeFrameFromLRUKChain(MainChainNodeType *first_occurrence_ptr) {
  if (first_occurrence_ptr == nullptr) return;
  MainChainNodeType *tmp;
  while (first_occurrence_ptr != nullptr) {
    tmp = first_occurrence_ptr;
    RemoveFromList(tmp);
    first_occurrence_ptr = first_occurrence_ptr->next_self_record;
    delete tmp;
  }
}

LRUKReplacer::MainChainNodeType *LRUKReplacer::AddRecordToMainChain(frame_id_t frame_id, size_t time_stamp,
                                                                    MainChainNodeType *last_node_in_main_chain) {
  MainChainNodeType *new_node = new LRUKReplacer::MainChainNodeType(frame_id, time_stamp, nullptr, nullptr, nullptr);
  if (last_node_in_main_chain != nullptr) last_node_in_main_chain->next_self_record = new_node;
  InsertAt(new_node, LRUK_chain_tail_guard->prev, LRUK_chain_tail_guard);
  return new_node;
}

bool LRUKReplacer::TryEvictExactFrame(frame_id_t frame_id) {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (!hash_for_record[frame_id].active) {
    return false;
  }
  if (!hash_for_record[frame_id].evitable) {
    return false;
  }
  LRUChainNodeType *node = hash_for_record[frame_id].node_in_LRU_chain;
  if (node != nullptr) RemoveFromList(node);
  delete node;
  hash_for_record[frame_id].node_in_LRU_chain = nullptr;
  RemoveWholeFrameFromLRUKChain(hash_for_record[frame_id].head_node_in_main_chain);
  hash_for_record[frame_id].active = false;
  current_evitable_count_--;
  return true;
}

bool LRUKReplacer::TryEvictLeastImportant(frame_id_t &frame_id) {
#ifdef ENABLE_ADVANCED_FEATURE
  latch.lock();
#endif
  if (current_evitable_count_ == 0) {
#ifdef ENABLE_ADVANCED_FEATURE
    latch.unlock();
#endif
    return false;
  }
  LRUChainNodeType *node = LRU_chain_head_guard->next;
  while (node != LRU_chain_tail_guard) {
    frame_id = node->frame_id;
    if (hash_for_record[frame_id].evitable) {
#ifdef ENABLE_ADVANCED_FEATURE
      latch.unlock();
#endif
      return TryEvictExactFrame(frame_id);
    }
    node = node->next;
  }
  MainChainNodeType *main_chain_node = LRUK_chain_head_guard->next;
  while (main_chain_node != LRUK_chain_tail_guard) {
    frame_id = main_chain_node->frame_id;
    if (hash_for_record[frame_id].evitable) {
#ifdef ENABLE_ADVANCED_FEATURE
      latch.unlock();
#endif
      return TryEvictExactFrame(frame_id);
    }
    main_chain_node = main_chain_node->next;
  }
#ifdef ENABLE_ADVANCED_FEATURE
  latch.unlock();
#endif
  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  current_timestamp_++;
  if (!hash_for_record[frame_id].active) {
    hash_for_record[frame_id].active = true;
    hash_for_record[frame_id].evitable = false;
    hash_for_record[frame_id].visit_count = 1;
    hash_for_record[frame_id].head_node_in_main_chain = hash_for_record[frame_id].tail_node_in_main_chain =
        AddRecordToMainChain(frame_id, current_timestamp_, nullptr);
    hash_for_record[frame_id].node_in_LRU_chain =
        new LRUChainNodeType(frame_id, LRU_chain_tail_guard->prev, LRU_chain_tail_guard);
    InsertAt(hash_for_record[frame_id].node_in_LRU_chain, LRU_chain_tail_guard->prev, LRU_chain_tail_guard);
  } else {
    hash_for_record[frame_id].visit_count++;
    MainChainNodeType *last_occurrence_ptr = hash_for_record[frame_id].tail_node_in_main_chain;
    MainChainNodeType *new_node_in_main_chain_ptr =
        AddRecordToMainChain(frame_id, current_timestamp_, last_occurrence_ptr);
    hash_for_record[frame_id].tail_node_in_main_chain = new_node_in_main_chain_ptr;
    if (hash_for_record[frame_id].node_in_LRU_chain != nullptr) {
      RemoveFromList(hash_for_record[frame_id].node_in_LRU_chain);
      delete hash_for_record[frame_id].node_in_LRU_chain;
      hash_for_record[frame_id].node_in_LRU_chain = nullptr;
    }
    if (hash_for_record[frame_id].visit_count < k_value) {
      hash_for_record[frame_id].node_in_LRU_chain =
          new LRUChainNodeType(frame_id, LRU_chain_tail_guard->prev, LRU_chain_tail_guard);
      InsertAt(hash_for_record[frame_id].node_in_LRU_chain, LRU_chain_tail_guard->prev, LRU_chain_tail_guard);
    }
    if (hash_for_record[frame_id].visit_count > k_value) {
      MainChainNodeType *first_occurrence_ptr = hash_for_record[frame_id].head_node_in_main_chain;
      RemoveFromList(first_occurrence_ptr);
      hash_for_record[frame_id].head_node_in_main_chain = first_occurrence_ptr->next_self_record;
      delete first_occurrence_ptr;
    }
  }
}

size_t LRUKReplacer::GetCurrentEvitableCount() { return current_evitable_count_; }