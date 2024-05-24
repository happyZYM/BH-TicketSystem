#ifndef TRANSACTION_MAINTENANCE_HPP
#define TRANSACTION_MAINTENANCE_HPP
#include <cstdint>
#include <vector>
#include "basic_defs.h"
#include "data.h"
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/config.h"
#include "storage/disk_manager.h"
#include "storage/driver.h"
#include "storage/single_value_storage.hpp"
#include "utils.h"
struct TransactionData {
  char trainID[21];
  char from_station_name[41];
  char to_station_name[41];
  uint8_t status;  // 0: in queue, 1: success, 2: refunded
  uint32_t leave_time_stamp;
  uint32_t arrive_time_stamp;
  uint32_t num;
  uint64_t total_price;
};
class TransactionManager : public DataDriverBase {
  struct queue_index_t {
    hash_t train_ID_hash;
    uint8_t running_offset;
    uint32_t id;
    inline bool operator<(const queue_index_t &rhs) const {
      if (train_ID_hash != rhs.train_ID_hash) return train_ID_hash < rhs.train_ID_hash;
      if (running_offset != rhs.running_offset) return running_offset < rhs.running_offset;
      return id < rhs.id;
    }
  };
  struct order_history_index_t {
    hash_t user_ID_hash;
    uint32_t id;
    inline bool operator<(const order_history_index_t &rhs) const {
      if (user_ID_hash != rhs.user_ID_hash) return user_ID_hash < rhs.user_ID_hash;
      return id > rhs.id;
    }
  };
  static const uint32_t queue_index_specai_id = 0;
  static const uint32_t order_history_index_special_id = -1;
  std::string data_file_identifier;
  std::string data_file_path;
  DiskManager *data_disk_manager;
  BufferPoolManager *data_bpm;
  SingleValueStorage<TransactionData> *data_storage;
  std::string queue_file_identifier;
  std::string queue_file_path;
  DiskManager *queue_disk_manager;
  BufferPoolManager *queue_bpm;
  BPlusTreeIndexer<queue_index_t, std::less<queue_index_t>> *queue_indexer;
  std::string order_history_file_identifier;
  std::string order_history_file_path;
  DiskManager *order_history_disk_manager;
  BufferPoolManager *order_history_bpm;
  BPlusTreeIndexer<order_history_index_t, std::less<order_history_index_t>> *order_history_indexer;

 public:
  // for satety, all the copy/move operations are deleted, please manage it using pointer
  inline TransactionManager(TransactionManager &&) = delete;
  inline TransactionManager &operator=(TransactionManager &&) = delete;
  inline TransactionManager(const TransactionManager &) = delete;
  inline TransactionManager &operator=(const TransactionManager &) = delete;
  inline TransactionManager(std::string data_file_identifier_, std::string data_file_path_,
                            std::string queue_file_identifier_, std::string queue_file_path_,
                            std::string order_history_file_identifier_, std::string order_history_file_path_)
      : data_file_identifier(std::move(data_file_identifier_)),
        data_file_path(std::move(data_file_path_)),
        queue_file_identifier(std::move(queue_file_identifier_)),
        queue_file_path(std::move(queue_file_path_)),
        order_history_file_identifier(std::move(order_history_file_identifier_)),
        order_history_file_path(std::move(order_history_file_path_)) {
    data_disk_manager = new DiskManager(data_file_path);
    data_bpm = new BufferPoolManager(100, 5, data_disk_manager);
    data_storage = new SingleValueStorage<TransactionData>(data_bpm);
    queue_disk_manager = new DiskManager(queue_file_path);
    queue_bpm = new BufferPoolManager(100, 5, queue_disk_manager);
    queue_indexer = new BPlusTreeIndexer<queue_index_t, std::less<queue_index_t>>(queue_bpm);
    order_history_disk_manager = new DiskManager(order_history_file_path);
    order_history_bpm = new BufferPoolManager(100, 5, order_history_disk_manager);
    order_history_indexer =
        new BPlusTreeIndexer<order_history_index_t, std::less<order_history_index_t>>(order_history_bpm);
  }
  inline ~TransactionManager() {
    delete data_storage;
    delete data_bpm;
    delete data_disk_manager;
    delete queue_indexer;
    delete queue_bpm;
    delete queue_disk_manager;
    delete order_history_indexer;
    delete order_history_bpm;
    delete order_history_disk_manager;
  }
  inline virtual sjtu::vector<FileEntry> ListFiles() override {
    sjtu::vector<FileEntry> res;
    res.push_back({data_file_identifier, data_file_path, data_disk_manager});
    res.push_back({queue_file_identifier, queue_file_path, queue_disk_manager});
    res.push_back({order_history_file_identifier, order_history_file_path, order_history_disk_manager});
    return res;
  }
  inline virtual void LockDownForCheckOut() override {
    delete data_storage;
    delete data_bpm;
    delete data_disk_manager;
    data_storage = nullptr;
    data_bpm = nullptr;
    data_disk_manager = nullptr;
    delete queue_indexer;
    delete queue_bpm;
    delete queue_disk_manager;
    queue_indexer = nullptr;
    queue_bpm = nullptr;
    queue_disk_manager = nullptr;
    delete order_history_indexer;
    delete order_history_bpm;
    delete order_history_disk_manager;
    order_history_indexer = nullptr;
    order_history_bpm = nullptr;
    order_history_disk_manager = nullptr;
  }
  inline virtual void Flush() override {
    if (data_storage == nullptr) return;
    data_storage->Flush();
    queue_indexer->Flush();
    order_history_indexer->Flush();
  }
  inline void PrepareTrainInfo(hash_t train_ID_hash, int total_days) {
    queue_index_t queue_index_for_query;
    queue_index_for_query.train_ID_hash = train_ID_hash;
    queue_index_for_query.running_offset = 0;
    queue_index_for_query.id = queue_index_specai_id;
    for (int i = 0; i < total_days; i++) {
      queue_index_for_query.running_offset = i;
      queue_indexer->Put(queue_index_for_query, 0);
    }
  }
  inline void PrepareUserInfo(hash_t user_ID_hash) {
    order_history_index_t order_history_index_for_query;
    order_history_index_for_query.user_ID_hash = user_ID_hash;
    order_history_index_for_query.id = order_history_index_special_id;
    order_history_indexer->Put(order_history_index_for_query, 0);
  }
  inline void AddOrder(std::string trainID, std::string from_station_name, std::string to_station_name, uint8_t status,
                       uint32_t leave_time_stamp, uint32_t arrive_time_stamp, uint32_t num, uint64_t total_price,
                       uint8_t running_date_offset, std::string username) {
    TransactionData tmp;
    strcpy(tmp.trainID, trainID.c_str());
    strcpy(tmp.from_station_name, from_station_name.c_str());
    strcpy(tmp.to_station_name, to_station_name.c_str());
    tmp.status = status;
    tmp.leave_time_stamp = leave_time_stamp;
    tmp.arrive_time_stamp = arrive_time_stamp;
    tmp.num = num;
    tmp.total_price = total_price;
    b_plus_tree_value_index_t data_id = data_storage->write(tmp);
    hash_t train_ID_hash = SplitMix64Hash(trainID);
    hash_t user_ID_hash = SplitMix64Hash(username);
    if (status == 0) {
      queue_index_t queue_index_for_query;
      queue_index_for_query.train_ID_hash = train_ID_hash;
      queue_index_for_query.running_offset = running_date_offset;
      queue_index_for_query.id = queue_index_specai_id;
      int new_id;
      {
        auto it_queue_query = queue_indexer->lower_bound(queue_index_for_query);
        new_id = it_queue_query.GetValue() + 1;
        it_queue_query.SetValue(new_id);
      }  // to release the lock
      queue_index_t queue_index;
      queue_index.train_ID_hash = queue_index_for_query.train_ID_hash;
      queue_index.running_offset = running_date_offset;
      queue_index.id = new_id;
      queue_indexer->Put(queue_index, data_id);
    }
    order_history_index_t order_history_index_for_query;
    order_history_index_for_query.user_ID_hash = user_ID_hash;
    order_history_index_for_query.id = order_history_index_special_id;
    int new_id;
    {
      auto it_order_history_query = order_history_indexer->lower_bound(order_history_index_for_query);
      new_id = it_order_history_query.GetValue() + 1;
      it_order_history_query.SetValue(new_id);
    }  // to release the lock
    order_history_index_t order_history_index;
    order_history_index.user_ID_hash = user_ID_hash;
    order_history_index.id = new_id;
    order_history_indexer->Put(order_history_index, data_id);
  }
  inline void FetchQueue(hash_t train_ID_hash, uint8_t running_date_offset,
                         std::vector<std::pair<b_plus_tree_value_index_t, uint32_t>> &res) {
    // warning: the validity of train_ID_hash is not checked
    queue_index_t queue_index_for_query;
    queue_index_for_query.train_ID_hash = train_ID_hash;
    queue_index_for_query.running_offset = running_date_offset;
    queue_index_for_query.id = queue_index_specai_id;
    auto it = queue_indexer->lower_bound_const(queue_index_for_query);
    int total_num = it.GetValue();
    res.resize(total_num);
    for (int i = 0; i < total_num; i++) {
      ++it;
      res[i].first = it.GetValue();
      res[i].second = it.GetKey().id;
    }
  }
  inline void RemoveOrderFromQueue(hash_t train_ID_hash, uint8_t running_date_offset, uint32_t id) {
    // warning: the validity of train_ID_hash is not checked
    queue_index_t queue_index_for_query;
    queue_index_for_query.train_ID_hash = train_ID_hash;
    queue_index_for_query.running_offset = running_date_offset;
    queue_index_for_query.id = id;
    queue_indexer->Remove(queue_index_for_query);
  }
  inline void FetchFullUserOrderHistory(hash_t user_ID_hash, std::vector<b_plus_tree_value_index_t> &res) {
    // warning: the validity of user_ID_hash is not checked
    order_history_index_t order_history_index_for_query;
    order_history_index_for_query.user_ID_hash = user_ID_hash;
    order_history_index_for_query.id = order_history_index_special_id;
    auto it = order_history_indexer->lower_bound_const(order_history_index_for_query);
    int total_num = it.GetValue();
    res.resize(total_num);
    for (int i = 0; i < total_num; i++) {
      ++it;
      res[i] = it.GetValue();
    }
  }
  inline b_plus_tree_value_index_t FetchSingleUserOrderHistory(hash_t user_ID_hash, int n, bool &success) {
    // warning: the validity of user_ID_hash is not checked
    order_history_index_t order_history_index_for_query;
    order_history_index_for_query.user_ID_hash = user_ID_hash;
    order_history_index_for_query.id = order_history_index_special_id;
    auto it = order_history_indexer->lower_bound_const(order_history_index_for_query);
    int total_num = it.GetValue();
    if (n > total_num) {
      success = false;
      return 0;
    }
    order_history_index_for_query.id = total_num - n + 1;
    auto dat_it = order_history_indexer->lower_bound_const(order_history_index_for_query);
    success = true;
    return dat_it.GetValue();
  }
  inline void FetchTransactionData(b_plus_tree_value_index_t idx, TransactionData &data) {
    // warning: the validity of idx is not checked
    LOG->debug("fetching transaction data with idx {}", idx);
    data_storage->read(data, idx);
  }
};
#endif