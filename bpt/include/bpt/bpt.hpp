#ifndef BPT_HPP
#define BPT_HPP
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <shared_mutex>
#include <vector>
#include "bpt/bpt_page.hpp"
#include "bpt/buffer_pool_manager.h"
#include "bpt/config.h"
/**
 * @brief B+ Tree Indexer
 * @warning The KeyType must can be stored byte by byte. As this is only the indexer, the type of value is always
 * b_plus_tree_value_index_t. And also, this is only the indexer, the value is not stored in the indexer, the value is
 * stored in the value file, and the BPlusTreeIndexer should not be used directly.
 */
template <typename KeyType, typename KeyComparator>
class BPlusTreeIndexer {
  typedef BPlusTreePage<KeyType> PageType;
  typedef ActualDataType<KeyType> _ActualDataType;
  typedef std::pair<KeyType, default_numeric_index_t> key_index_pair_t;
  // typedef std::pair<KeyType, b_plus_tree_value_index_t> value_type;

 private:
  struct PositionSignType {
    std::vector<std::pair<BasicPageGuard, in_page_key_count_t>> path;
    bool is_end{false};
  };
  PositionSignType FindPosition(const KeyType &key) {  // Finish Design
    if (root_page_id == 0) {
      // special case for the empty tree
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
    res.path.push_back(std::make_pair(std::move(current_page_guard), nxt));
    while ((res.path.back().first.template As<PageType>()->data.page_status & PageStatusType::LEAF) == 0) {
      default_numeric_index_t nxt_page_id;
      in_page_key_count_t internal_id = res.path.back().second;
      if (internal_id < _ActualDataType::kMaxKeyCount)
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
    if (nxt == res.path.back().first.template As<PageType>()->data.key_count)
      res.is_end = true;
    else
      res.is_end = false;
    return res;
  }
  void InsertFixUpLoopPartA(PositionSignType &pos, BasicPageGuard &parent_page_guard, BasicPageGuard &new_page_guard,
                            BasicPageGuard &page_guard, default_numeric_index_t new_page_id) {
    pos.path[pos.path.size() - 2].second++;
    // now check we are able to "insert" (new_page_guard.template
    // As<PageType>()->data.p_data[new_page_guard.template As<PageType>()->data.key_count - 1].first, new_page_id)
    // at pos
    if (parent_page_guard.template As<PageType>()->data.key_count < _ActualDataType::kMaxKeyCount) {
      // Has enough space, reach end, just insert it
      // first, manually move the last pointer
      parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
          page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first;
      if (parent_page_guard.template As<PageType>()->data.key_count == _ActualDataType::kMaxKeyCount - 1) {
        parent_page_guard.template AsMut<PageType>()->data.p_n =
            parent_page_guard.template As<PageType>()
                ->data.p_data[parent_page_guard.template As<PageType>()->data.key_count]
                .second;
      } else {
        parent_page_guard.template AsMut<PageType>()
            ->data.p_data[parent_page_guard.template As<PageType>()->data.key_count + 1]
            .second = parent_page_guard.template As<PageType>()
                          ->data.p_data[parent_page_guard.template As<PageType>()->data.key_count]
                          .second;
      }
      // Then, use memmove to move the key_point pairs
      // fprintf(stderr, "parent_page_guard.template As<PageType>()->data.key_count = %d\n",
      //         (int)parent_page_guard.template As<PageType>()->data.key_count);
      if (pos.path[pos.path.size() - 2].second < parent_page_guard.template As<PageType>()->data.key_count) {
        memmove(parent_page_guard.template AsMut<PageType>()->data.p_data + pos.path[pos.path.size() - 2].second + 1,
                parent_page_guard.template As<PageType>()->data.p_data + pos.path[pos.path.size() - 2].second,
                (parent_page_guard.template As<PageType>()->data.key_count - pos.path[pos.path.size() - 2].second) *
                    sizeof(key_index_pair_t));
      }
      // Then Set the key_point pair
      if (pos.path[pos.path.size() - 2].second < _ActualDataType::kMaxKeyCount) {
        parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second] =
            std::make_pair(new_page_guard.template As<PageType>()
                               ->data.p_data[new_page_guard.template As<PageType>()->data.key_count - 1]
                               .first,
                           new_page_id);
      } else {
        // just set p_n
        parent_page_guard.template AsMut<PageType>()->data.p_n = new_page_id;
      }
      parent_page_guard.template AsMut<PageType>()->data.key_count++;
      return;
    }
    // TODO: process and prepare for next round
    assert((pos.path.size() == 2) ==
           ((parent_page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT) != 0));
    /**
     * Step 1: split current page
     * Step 2: Check if current page is root. If current page is root, create a new root, update and exit
     * Step 3: Otherwise, update parent page, and continue the loop
     */
    key_index_pair_t new_entry_backup =
        std::make_pair(new_page_guard.template As<PageType>()
                           ->data.p_data[new_page_guard.template As<PageType>()->data.key_count - 1]
                           .first,
                       new_page_id);
    KeyType key_to_update_backup =
        page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first;
    InsertFixUpLoopPartB(pos, parent_page_guard, new_entry_backup, key_to_update_backup);
  }
  void InsertFixUpLoopPartB(PositionSignType &pos, BasicPageGuard &page_guard, const key_index_pair_t &new_entry_backup,
                            const KeyType &key_to_update_backup) {
    default_numeric_index_t new_page_id;
    auto new_page_guard = std::move(bpm->NewPageGuarded(&new_page_id));
    new_page_guard.template AsMut<PageType>()->data.page_status = PageStatusType::INTERNAL;
    // Now begin spliting. It is expected that the new page has _ActualDataType::kMinNumberOfKeysForLeaf keys
    if (pos.path[pos.path.size() - 2].second - 1 == _ActualDataType::kMaxKeyCount) {
      // In this case, first, move the last _ActualDataType::kMinNumberOfKeysForLeaf-1 keys to the new page
      memmove(new_page_guard.template AsMut<PageType>()->data.p_data,
              page_guard.template As<PageType>()->data.p_data + _ActualDataType::kMaxKeyCount -
                  (_ActualDataType::kMinNumberOfKeysForLeaf - 1),
              (_ActualDataType::kMinNumberOfKeysForLeaf - 1) * sizeof(key_index_pair_t));
      new_page_guard.template AsMut<PageType>()->data.p_data[_ActualDataType::kMinNumberOfKeysForLeaf - 1].second =
          page_guard.template As<PageType>()->data.p_n;
      new_page_guard.template AsMut<PageType>()->data.p_data[_ActualDataType::kMinNumberOfKeysForLeaf - 1].first =
          key_to_update_backup;
      new_page_guard.template AsMut<PageType>()->data.p_data[_ActualDataType::kMinNumberOfKeysForLeaf].second =
          new_entry_backup.second;
      new_page_guard.template AsMut<PageType>()->data.key_count = _ActualDataType::kMinNumberOfKeysForLeaf;
      page_guard.template AsMut<PageType>()->data.key_count -= _ActualDataType::kMinNumberOfKeysForLeaf - 1;
    } else {
      page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
          key_to_update_backup;
      // now, we need to "insert" the new_entry_backup to position pos.path[pos.path.size() - 2].second
      new_page_guard.template AsMut<PageType>()->data.p_data[_ActualDataType::kMinNumberOfKeysForLeaf].second =
          page_guard.template As<PageType>()->data.p_n;
      default_numeric_index_t it_dest = _ActualDataType::kMinNumberOfKeysForLeaf - 1;
      default_numeric_index_t it_src = _ActualDataType::kMaxKeyCount - 1;
      bool has_processed = false;
      for (; it_dest != kInvalidNumericIndex; --it_dest) {
        if (!has_processed && pos.path[pos.path.size() - 2].second == it_src + 1) {
          new_page_guard.template AsMut<PageType>()->data.p_data[it_dest] = new_entry_backup;
          has_processed = true;
          continue;
        }
        new_page_guard.template AsMut<PageType>()->data.p_data[it_dest] =
            page_guard.template As<PageType>()->data.p_data[it_src];
        --it_src;
      }
      if (pos.path[pos.path.size() - 2].second <=
          _ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf) {
        memmove(page_guard.template AsMut<PageType>()->data.p_data + pos.path[pos.path.size() - 2].second + 1,
                page_guard.template As<PageType>()->data.p_data + pos.path[pos.path.size() - 2].second,
                ((_ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf) -
                 pos.path[pos.path.size() - 2].second) *
                    sizeof(key_index_pair_t));
        page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second] = new_entry_backup;
      }
      new_page_guard.template AsMut<PageType>()->data.key_count = _ActualDataType::kMinNumberOfKeysForLeaf;
      page_guard.template AsMut<PageType>()->data.key_count -= _ActualDataType::kMinNumberOfKeysForLeaf - 1;
    }
    // fprintf(stderr, "Evicting page %d\n", (int)pos.path.back().first.PageId());
    // fprintf(stderr, "page id of page_guard = %d\n", (int)page_guard.PageId());
    pos.path.pop_back();
    // fprintf(stderr, "the page id of the res page in pos %d\n", (int)pos.path.back().first.PageId());
    if (pos.path.size() == 1) {
      // we have split the root page, update and quit
      page_guard.template AsMut<PageType>()->data.page_status &= ~PageStatusType::ROOT;
      BasicPageGuard new_root_page_guard = bpm->NewPageGuarded(&root_page_id);
      new_root_page_guard.AsMut<PageType>()->data.page_status = PageStatusType::ROOT;
      new_root_page_guard.AsMut<PageType>()->data.key_count = 1;
      new_root_page_guard.AsMut<PageType>()->data.p_data[0] = std::make_pair(
          page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first,
          page_guard.PageId());
      new_root_page_guard.AsMut<PageType>()->data.p_data[1] = std::make_pair(KeyType(), new_page_id);
      return;
    }
    auto &parent_page_guard = pos.path[pos.path.size() - 2].first;
    InsertFixUpLoopPartA(pos, parent_page_guard, new_page_guard, page_guard, new_page_id);
  }
  void InsertEntryAt(PositionSignType &pos, const KeyType &key, b_plus_tree_value_index_t value,
                     bool is_fixing_up_recursive = false) {
    // fprintf(stderr, "_ActualDataType::kMaxKeyCount = %d\n", (int)_ActualDataType::kMaxKeyCount);
    if (siz == 0) {
      // special case for the first entry
      BasicPageGuard new_page_guard = bpm->NewPageGuarded(&root_page_id);
      new_page_guard.AsMut<PageType>()->data.page_status = PageStatusType::ROOT | PageStatusType::LEAF;
      new_page_guard.AsMut<PageType>()->data.key_count = 1;
      new_page_guard.AsMut<PageType>()->data.p_data[0] = std::make_pair(key, value);
      new_page_guard.AsMut<PageType>()->data.p_n = 0;
      return;
    }
    auto &page_guard = pos.path.back().first;
    if (page_guard.template As<PageType>()->data.key_count < _ActualDataType::kMaxKeyCount) {
      // case 1: the page has enough space
      memmove(page_guard.template AsMut<PageType>()->data.p_data + pos.path.back().second + 1,
              page_guard.template As<PageType>()->data.p_data + pos.path.back().second,
              (page_guard.template As<PageType>()->data.key_count - pos.path.back().second) * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.p_data[pos.path.back().second] = std::make_pair(key, value);
      page_guard.template AsMut<PageType>()->data.key_count++;
      // fprintf(stderr, "page_guard.template As<PageType>()->data.key_count = %d\n",
      // (int)page_guard.template As<PageType>()->data.key_count);
      return;
    }
    // In our case, the tree is not too high, so we do not consider borrowing from siblings, we just split the page.
    // We first construct a new page, and then move half of the keys to the new page.
    // The check if we split the root page, we just handle it.
    // Otherwise, what we need to do is modify the parent page, then "insert" a new key to the parent page
    page_id_t new_page_id;
    BasicPageGuard new_page_guard = bpm->NewPageGuarded(&new_page_id);
    // Then move the last kMinNumberOfKeysForLeaf keys(including newly inserted) to the new page
    if (!is_fixing_up_recursive)
      new_page_guard.AsMut<PageType>()->data.page_status = PageStatusType::LEAF;
    else
      new_page_guard.AsMut<PageType>()->data.page_status = PageStatusType::INTERNAL;
    new_page_guard.AsMut<PageType>()->data.key_count = _ActualDataType::kMinNumberOfKeysForLeaf;
    page_guard.template AsMut<PageType>()->data.key_count -= _ActualDataType::kMinNumberOfKeysForLeaf;
    if (!is_fixing_up_recursive)
      new_page_guard.AsMut<PageType>()->data.p_n = page_guard.template As<PageType>()->data.p_n;
    if (!is_fixing_up_recursive) page_guard.template AsMut<PageType>()->data.p_n = new_page_id;
    if (pos.path.back().second <= _ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf) {
      // the new key is in the first half
      memmove(new_page_guard.template AsMut<PageType>()->data.p_data,
              page_guard.template As<PageType>()->data.p_data + _ActualDataType::kMaxKeyCount -
                  _ActualDataType::kMinNumberOfKeysForLeaf,
              _ActualDataType::kMinNumberOfKeysForLeaf * sizeof(key_index_pair_t));
      memmove(page_guard.template AsMut<PageType>()->data.p_data + pos.path.back().second + 1,
              page_guard.template As<PageType>()->data.p_data + pos.path.back().second,
              (page_guard.template As<PageType>()->data.key_count - pos.path.back().second) * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.p_data[pos.path.back().second] = std::make_pair(key, value);
      page_guard.template AsMut<PageType>()->data.key_count++;
    } else {
      // the new key is in the second half
      memmove(
          new_page_guard.template AsMut<PageType>()->data.p_data,
          page_guard.template As<PageType>()->data.p_data + _ActualDataType::kMaxKeyCount -
              _ActualDataType::kMinNumberOfKeysForLeaf + 1,
          (pos.path.back().second - (_ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf + 1)) *
              sizeof(key_index_pair_t));
      new_page_guard.template AsMut<PageType>()
          ->data.p_data[pos.path.back().second -
                        (_ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf + 1)] =
          std::make_pair(key, value);
      memmove(new_page_guard.template AsMut<PageType>()->data.p_data + pos.path.back().second -
                  (_ActualDataType::kMaxKeyCount - _ActualDataType::kMinNumberOfKeysForLeaf + 1) + 1,
              page_guard.template As<PageType>()->data.p_data + pos.path.back().second,
              (_ActualDataType::kMaxKeyCount - pos.path.back().second) * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.key_count++;
    }
    if (page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT) {
      // special case for the root page being splited
      page_guard.template AsMut<PageType>()->data.page_status &= ~PageStatusType::ROOT;
      BasicPageGuard new_root_page_guard = bpm->NewPageGuarded(&root_page_id);
      new_root_page_guard.AsMut<PageType>()->data.page_status = PageStatusType::ROOT;
      new_root_page_guard.AsMut<PageType>()->data.key_count = 1;
      new_root_page_guard.AsMut<PageType>()->data.p_data[0] = std::make_pair(
          page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first,
          page_guard.PageId());
      new_root_page_guard.AsMut<PageType>()->data.p_data[1] = std::make_pair(KeyType(), new_page_id);
      // fprintf(stderr, "new_page_guard.AsMut<PageType>()->data.key_count = %d\n",
      // (int)new_page_guard.AsMut<PageType>()->data.key_count);
      return;
    }
    assert(pos.path.size() >= 2);
    auto &parent_page_guard = pos.path[pos.path.size() - 2].first;
    bool is_in_right_skew_path = false;
    // if (pos.path[pos.path.size() - 2].second == parent_page_guard.template As<PageType>()->data.key_count) {
    //   is_in_right_skew_path = true;
    // }
    if (pos.path.size() == 2 || pos.path[pos.path.size() - 3].second ==
                                    pos.path[pos.path.size() - 3].first.template As<PageType>()->data.key_count) {
      is_in_right_skew_path = true;
    }
    if (is_in_right_skew_path) {
      InsertFixUpLoopPartA(pos, parent_page_guard, new_page_guard, page_guard, new_page_id);
      return;
    }
    parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second].first =
        page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first;
    pos.path[pos.path.size() - 2].second++;
    pos.path.pop_back();
    // fprintf(stderr, "begin processing recursively\n");
    InsertEntryAt(pos,
                  new_page_guard.template As<PageType>()
                      ->data.p_data[new_page_guard.template As<PageType>()->data.key_count - 1]
                      .first,
                  new_page_id, true);
    return;
  }
  void RemoveEntryInRightSkewPath(PositionSignType &pos) {
    auto &page_guard = pos.path.back().first;
    bool has_enough_keys = false;
    if (page_guard.template As<PageType>()->data.key_count > _ActualDataType::kMinNumberOfKeysForLeaf ||
        (page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT) != 0) {
      // case 1: the page has enough keys
      has_enough_keys = true;
    }
    if (pos.path.back().second == page_guard.template As<PageType>()->data.key_count) {
      // The "entry" to remove is just a past-the-end pointer
      page_guard.template AsMut<PageType>()->data.key_count--;
    } else {
      // The "entry" to remove is a key-val pair
      memmove(
          page_guard.template AsMut<PageType>()->data.p_data + pos.path.back().second,
          page_guard.template As<PageType>()->data.p_data + pos.path.back().second + 1,
          (page_guard.template As<PageType>()->data.key_count - pos.path.back().second - 1) * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.key_count--;
      if (page_guard.template AsMut<PageType>()->data.key_count + 1 < _ActualDataType::kMaxKeyCount) {
        page_guard.template AsMut<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count].second =
            page_guard.template As<PageType>()
                ->data.p_data[page_guard.template As<PageType>()->data.key_count + 1]
                .second;
      } else {
        page_guard.template AsMut<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count].second =
            page_guard.template As<PageType>()->data.p_n;
      }
    }
    if (has_enough_keys) {
      if (page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT &&
          page_guard.template As<PageType>()->data.key_count == 0) {
        // special case for the root page
        root_page_id = page_guard.template As<PageType>()->data.p_data[0].second;
        BasicPageGuard root_page_guard = bpm->FetchPageBasic(root_page_id);
        root_page_guard.template AsMut<PageType>()->data.page_status |= PageStatusType::ROOT;
        page_id_t page_to_delete = page_guard.PageId();
        pos.path.clear();  // all page_guards are invalid now
        bpm->DeletePage(page_to_delete);
        return;
      }
      return;
    }
    assert(pos.path.size() >= 2);
    assert(pos.path[pos.path.size() - 2].second > 0);
    page_id_t possible_prev_page_id = 0;
    auto &parent_page_guard = pos.path[pos.path.size() - 2].first;
    possible_prev_page_id =
        parent_page_guard.template As<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].second;
    BasicPageGuard prev_page_guard = std::move(bpm->FetchPageBasic(possible_prev_page_id));
    if (prev_page_guard.As<PageType>()->data.key_count > _ActualDataType::kMinNumberOfKeysForLeaf) {
      // borrow from prev
      // first, set the past-the-end pointer
      page_guard.template AsMut<PageType>()
          ->data.p_data[page_guard.template As<PageType>()->data.key_count + 1]
          .second =
          page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count].second;
      memmove(page_guard.template AsMut<PageType>()->data.p_data + 1, page_guard.template As<PageType>()->data.p_data,
              page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.p_data[0] =
          prev_page_guard.template As<PageType>()
              ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1];
      page_guard.template AsMut<PageType>()->data.key_count++;
      prev_page_guard.template AsMut<PageType>()->data.key_count--;
      parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
          prev_page_guard.template As<PageType>()
              ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1]
              .first;
      return;
    }
    // now we have no choice but to merge self into prev
    memmove(prev_page_guard.template AsMut<PageType>()->data.p_data +
                prev_page_guard.template As<PageType>()->data.key_count,
            page_guard.template As<PageType>()->data.p_data,
            page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
    prev_page_guard.template AsMut<PageType>()->data.key_count += page_guard.template As<PageType>()->data.key_count;
    if (page_guard.template As<PageType>()->data.key_count < _ActualDataType::kMaxKeyCount) {
      if (prev_page_guard.template As<PageType>()->data.key_count < _ActualDataType::kMaxKeyCount) {
        prev_page_guard.template AsMut<PageType>()
            ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count]
            .second =
            page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count].second;
      } else {
        prev_page_guard.template AsMut<PageType>()->data.p_n =
            page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count].second;
      }
    } else {
      if (prev_page_guard.template As<PageType>()->data.key_count < _ActualDataType::kMaxKeyCount) {
        prev_page_guard.template AsMut<PageType>()
            ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count]
            .second = page_guard.template As<PageType>()->data.p_n;
      } else {
        prev_page_guard.template AsMut<PageType>()->data.p_n = page_guard.template As<PageType>()->data.p_n;
      }
    }
    parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
        prev_page_guard.template As<PageType>()
            ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1]
            .first;
    pos.path.pop_back();  // page_guard is no longer valid
    RemoveEntryInRightSkewPath(pos);
    return;
  }
  void RemoveEntryAt(PositionSignType &pos, bool is_fixing_up_recursive = false) {
    if (siz == 1) {
      // special case for the last entry
      bpm->DeletePage(root_page_id);
      root_page_id = 0;
      return;
    }
    auto &page_guard = pos.path.back().first;
    bool has_enough_keys = false;
    if (page_guard.template As<PageType>()->data.key_count > _ActualDataType::kMinNumberOfKeysForLeaf ||
        (page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT) != 0) {
      // case 1: the page has enough keys
      has_enough_keys = true;
    }
    memmove(
        page_guard.template AsMut<PageType>()->data.p_data + pos.path.back().second,
        page_guard.template As<PageType>()->data.p_data + pos.path.back().second + 1,
        (page_guard.template As<PageType>()->data.key_count - pos.path.back().second - 1) * sizeof(key_index_pair_t));
    page_guard.template AsMut<PageType>()->data.key_count--;
    if (has_enough_keys) {
      if (page_guard.template As<PageType>()->data.page_status & PageStatusType::ROOT &&
          page_guard.template As<PageType>()->data.key_count == 0) {
        // special case for the root page
        root_page_id = page_guard.template As<PageType>()->data.p_data[0].second;
        BasicPageGuard root_page_guard = bpm->FetchPageBasic(root_page_id);
        root_page_guard.template AsMut<PageType>()->data.page_status |= PageStatusType::ROOT;
        page_id_t page_to_delete = page_guard.PageId();
        pos.path.clear();  // all page_guards are invalid now
        bpm->DeletePage(page_to_delete);
        return;
      }
      return;
    }
    assert(pos.path.size() >= 2);
    // First, check if we can borrow from siblings. If we can, we just borrow from siblings. Otherwise, we just merge.
    page_id_t possible_prev_page_id = 0, possible_next_page_id = 0;
    auto &parent_page_guard = pos.path[pos.path.size() - 2].first;
    bool is_in_right_skew_path = false;
    // if (pos.path[pos.path.size() - 2].second == parent_page_guard.template As<PageType>()->data.key_count) {
    //   is_in_right_skew_path = true;
    // }
    if (pos.path.size() == 2 || pos.path[pos.path.size() - 3].second ==
                                    pos.path[pos.path.size() - 3].first.template As<PageType>()->data.key_count) {
      is_in_right_skew_path = true;
    }
    if (is_in_right_skew_path) {
      if (pos.path[pos.path.size() - 2].second < parent_page_guard.template As<PageType>()->data.key_count) {
        if (pos.path[pos.path.size() - 2].second + 1 < _ActualDataType::kMaxKeyCount)
          possible_next_page_id =
              parent_page_guard.template As<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second + 1].second;
        else
          possible_next_page_id = parent_page_guard.template As<PageType>()->data.p_n;
      }
    } else {
      if (pos.path[pos.path.size() - 2].second < parent_page_guard.template As<PageType>()->data.key_count - 1)
        possible_next_page_id =
            parent_page_guard.template As<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second + 1].second;
    }
    if (pos.path[pos.path.size() - 2].second > 0)
      possible_prev_page_id =
          parent_page_guard.template As<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].second;
    if (possible_prev_page_id != 0) {
      BasicPageGuard prev_page_guard = std::move(bpm->FetchPageBasic(possible_prev_page_id));
      if (prev_page_guard.As<PageType>()->data.key_count > _ActualDataType::kMinNumberOfKeysForLeaf) {
        // borrow from prev
        memmove(page_guard.template AsMut<PageType>()->data.p_data + 1, page_guard.template As<PageType>()->data.p_data,
                page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
        page_guard.template AsMut<PageType>()->data.p_data[0] =
            prev_page_guard.template As<PageType>()
                ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1];
        page_guard.template AsMut<PageType>()->data.key_count++;
        prev_page_guard.template AsMut<PageType>()->data.key_count--;
        parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
            prev_page_guard.template As<PageType>()
                ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1]
                .first;
        return;
      }
    }
    if (possible_next_page_id != 0) {
      BasicPageGuard next_page_guard = std::move(bpm->FetchPageBasic(possible_next_page_id));
      if (next_page_guard.As<PageType>()->data.key_count > _ActualDataType::kMinNumberOfKeysForLeaf) {
        // borrow from next
        page_guard.template AsMut<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count] =
            next_page_guard.template As<PageType>()->data.p_data[0];
        page_guard.template AsMut<PageType>()->data.key_count++;
        next_page_guard.template AsMut<PageType>()->data.key_count--;
        parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second].first =
            page_guard.template As<PageType>()
                ->data.p_data[page_guard.template As<PageType>()->data.key_count - 1]
                .first;
        memmove(next_page_guard.template AsMut<PageType>()->data.p_data,
                next_page_guard.template As<PageType>()->data.p_data + 1,
                next_page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
        if (is_fixing_up_recursive &&
            pos.path[pos.path.size() - 2].second + 1 == parent_page_guard.template As<PageType>()->data.key_count) {
          if (next_page_guard.template As<PageType>()->data.key_count == _ActualDataType::kMaxKeyCount - 1) {
            // special process for meaningful p_n
            next_page_guard.template AsMut<PageType>()
                ->data.p_data[next_page_guard.template As<PageType>()->data.key_count]
                .second = next_page_guard.template As<PageType>()->data.p_n;
          } else {
            // special process for past-the-end p_i
            next_page_guard.template AsMut<PageType>()
                ->data.p_data[next_page_guard.template As<PageType>()->data.key_count]
                .second = next_page_guard.template As<PageType>()
                              ->data.p_data[next_page_guard.template As<PageType>()->data.key_count + 1]
                              .second;
          }
        }
        return;
      }
    }
    if (possible_prev_page_id != 0) {
      // merge self into prev
      BasicPageGuard prev_page_guard = std::move(bpm->FetchPageBasic(possible_prev_page_id));
      prev_page_guard.template AsMut<PageType>()->data.p_n = page_guard.template As<PageType>()->data.p_n;
      memmove(prev_page_guard.template AsMut<PageType>()->data.p_data +
                  prev_page_guard.template As<PageType>()->data.key_count,
              page_guard.template As<PageType>()->data.p_data,
              page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
      prev_page_guard.template AsMut<PageType>()->data.key_count += page_guard.template As<PageType>()->data.key_count;
      parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second - 1].first =
          prev_page_guard.template As<PageType>()
              ->data.p_data[prev_page_guard.template As<PageType>()->data.key_count - 1]
              .first;
      page_id_t current_page_id = page_guard.PageId();
      pos.path.pop_back();  // page_guard is no longer valid
      bpm->DeletePage(current_page_id);
      if (!is_in_right_skew_path) {
        // we need to update the parent page
        RemoveEntryAt(pos, true);
        return;
      }
      RemoveEntryInRightSkewPath(pos);
      return;
    }
    if (possible_next_page_id != 0) {
      // merge next into self
      assert(possible_prev_page_id == 0);
      BasicPageGuard next_page_guard = std::move(bpm->FetchPageBasic(possible_next_page_id));
      if (is_fixing_up_recursive &&
          pos.path[pos.path.size() - 2].second + 1 == parent_page_guard.template As<PageType>()->data.key_count) {
        // the next page has past-the-end pointer
        size_t intended_dest = next_page_guard.template As<PageType>()->data.key_count +
                               page_guard.template As<PageType>()->data.key_count;
        if (intended_dest == _ActualDataType::kMaxKeyCount) {
          page_guard.template AsMut<PageType>()->data.p_n =
              next_page_guard.template As<PageType>()
                  ->data.p_data[next_page_guard.template As<PageType>()->data.key_count]
                  .second;
        } else {
          page_guard.template AsMut<PageType>()->data.p_data[intended_dest].second =
              next_page_guard.template As<PageType>()
                  ->data.p_data[next_page_guard.template As<PageType>()->data.key_count]
                  .second;
        }
      }
      if (!is_fixing_up_recursive) {
        // update the p_n
        page_guard.template AsMut<PageType>()->data.p_n = next_page_guard.template As<PageType>()->data.p_n;
      }
      memmove(page_guard.template AsMut<PageType>()->data.p_data + page_guard.template As<PageType>()->data.key_count,
              next_page_guard.template As<PageType>()->data.p_data,
              next_page_guard.template As<PageType>()->data.key_count * sizeof(key_index_pair_t));
      page_guard.template AsMut<PageType>()->data.key_count += next_page_guard.template As<PageType>()->data.key_count;
      parent_page_guard.template AsMut<PageType>()->data.p_data[pos.path[pos.path.size() - 2].second].first =
          page_guard.template As<PageType>()->data.p_data[page_guard.template As<PageType>()->data.key_count - 1].first;
      page_id_t page_id_to_delete = next_page_guard.PageId();
      pos.path.pop_back();  // page_guard is no longer valid
      pos.path.back().second++;
      bpm->DeletePage(page_id_to_delete);
      if (!is_in_right_skew_path) {
        // we need to update the parent page
        RemoveEntryAt(pos, true);
        return;
      }
      RemoveEntryInRightSkewPath(pos);
      return;
    }
    throw std::runtime_error("No sibling found!");
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
    const KeyType &GetKey() const {
#ifdef ENABLE_ADVANCED_FEATURE
      std::shared_lock<std::shared_mutex> lock_guard(domain->latch);
#endif
      return guard.As<PageType>()->data.p_data[internal_offset].first;
    }
    const b_plus_tree_value_index_t &GetValue() {
#ifdef ENABLE_ADVANCED_FEATURE
      std::shared_lock<std::shared_mutex> lock_guard(domain->latch);
#endif
      return guard.As<PageType>()->data.p_data[internal_offset].second;
    }
    bool operator==(iterator &that) {
      return domain == that.domain && is_end == that.is_end &&
             (is_end || (guard.PageId() == that.guard.PageId() && internal_offset == that.internal_offset));
    }
    void SetValue(b_plus_tree_value_index_t new_value) {
#ifdef ENABLE_ADVANCED_FEATURE
      std::unique_lock<std::shared_mutex> lock_guard(domain->latch);
#endif
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
    const KeyType &GetKey() {
#ifdef ENABLE_ADVANCED_FEATURE
      std::shared_lock<std::shared_mutex> lock_guard(domain->latch);
#endif
      return guard.As<PageType>()->data.p_data[internal_offset].first;
    }
    const b_plus_tree_value_index_t &GetValue() {
#ifdef ENABLE_ADVANCED_FEATURE
      std::shared_lock<std::shared_mutex> lock_guard(domain->latch);
#endif
      return guard.As<PageType>()->data.p_data[internal_offset].second;
    }
    bool operator==(const_iterator &that) {
      return domain == that.domain && is_end == that.is_end &&
             (is_end || (guard.PageId() == that.guard.PageId() && internal_offset == that.internal_offset));
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
  ~BPlusTreeIndexer() { Flush(); }
  iterator end() {  // Finish Design
    iterator res;
    res.domain = this;
    res.is_end = true;
    return res;
  }
  const_iterator end_const() {  // Finish Design
    const_iterator res;
    res.domain = this;
    res.is_end = true;
    return res;
  }
  iterator lower_bound(const KeyType &key) {  // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::shared_lock<std::shared_mutex> guard(latch);
#endif
    PositionSignType pos(std::move(FindPosition(key)));
    iterator res;
    res.domain = this;
    res.is_end = pos.is_end;
    if (res.is_end) return res;
    res.guard = bpm->FetchPageWrite(pos.path.back().first.PageId());
    res.internal_offset = pos.path.back().second;
    return res;
  }
  const_iterator lower_bound_const(const KeyType &key) {  // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::shared_lock<std::shared_mutex> guard(latch);
#endif
    PositionSignType pos(std::move(FindPosition(key)));
    const_iterator res;
    res.domain = this;
    res.is_end = pos.is_end;
    if (res.is_end) return res;
    res.guard = bpm->FetchPageRead(pos.path.back().first.PageId());
    res.internal_offset = pos.path.back().second;
    return res;
  }
  b_plus_tree_value_index_t Get(const KeyType &key) {  // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::shared_lock<std::shared_mutex> guard(latch);
#endif
    auto it = lower_bound_const(key);
    if (it == end_const()) return kInvalidValueIndex;
    if (key_cmp(key, it.GetKey())) return kInvalidValueIndex;
    return it.GetValue();
  }
  bool Put(const KeyType &key, b_plus_tree_value_index_t value) {  // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::unique_lock<std::shared_mutex> guard(latch);
#endif
    PositionSignType pos(std::move(FindPosition(key)));
    if (!pos.is_end &&
        !key_cmp(key, pos.path.back().first.template As<PageType>()->data.p_data[pos.path.back().second].first)) {
      pos.path.back().first.template AsMut<PageType>()->data.p_data[pos.path.back().second].second = value;
      return false;
    }
    InsertEntryAt(pos, key, value);
    ++siz;
    return true;
  }
  bool Remove(const KeyType &key) {  // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::unique_lock<std::shared_mutex> guard(latch);
#endif
    PositionSignType pos(std::move(FindPosition(key)));
    if (pos.is_end) return false;
    if (key_cmp(key, pos.path.back().first.template As<PageType>()->data.p_data[pos.path.back().second].first))
      return false;
    RemoveEntryAt(pos);
    --siz;
    return true;
  }
  size_t Size() { return siz; }  // Finish Design
  void Flush() {                 // Finish Design
#ifdef ENABLE_ADVANCED_FEATURE
    std::unique_lock<std::shared_mutex> guard(latch);
#endif
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
#ifdef ENABLE_ADVANCED_FEATURE
  std::shared_mutex latch;
#endif
  BufferPoolManager *bpm;
  char *raw_data_memory;
};
template <typename KeyType, typename KeyComparator>
KeyComparator BPlusTreeIndexer<KeyType, KeyComparator>::key_cmp = KeyComparator();
#endif  // BPT_HPP