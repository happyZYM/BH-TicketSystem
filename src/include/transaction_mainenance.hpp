#ifndef TRANSACTION_MAINTENANCE_HPP
#define TRANSACTION_MAINTENANCE_HPP
#include <cstdint>
#include <vector>
#include "basic_defs.h"
#include "data.h"
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/disk_manager.h"
#include "storage/driver.h"
#include "storage/single_value_storage.hpp"
struct TransactionData {
  char trainID[21];
  char from_station_name[41];
  char to_station_name[41];
  uint8_t status;
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
};
#endif