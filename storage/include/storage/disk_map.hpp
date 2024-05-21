#ifndef DISK_MAP_H
#define DISK_MAP_H
#include <algorithm>
#include <string>
#include <utility>
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/driver.h"
#include "storage/single_value_storage.hpp"
template <typename Key, typename Value, typename Compare = std::less<Key>>
class DiskMap : public DataDriverBase {
  std::string index_file_identifier;
  std::string index_file_path;
  DiskManager *index_disk_manager;
  BufferPoolManager *index_bpm;
  BPlusTreeIndexer<Key, Compare> *indexer;
  std::string data_file_identifier;
  std::string data_file_path;
  DiskManager *data_disk_manager;
  BufferPoolManager *data_bpm;
  SingleValueStorage<Value> *data_storage;

 public:
  // for satety, all the copy/move operations are deleted, please manage it using pointer
  DiskMap(std::string index_file_identifier_, std::string index_file_path_, std::string data_file_identifier_,
          std::string data_file_path_)
      : index_file_identifier(std::move(index_file_identifier_)),
        index_file_path(std::move(index_file_path_)),
        data_file_identifier(std::move(data_file_identifier_)),
        data_file_path(std::move(data_file_path_)) {
    index_disk_manager = new DiskManager(index_file_path);
    index_bpm = new BufferPoolManager(100, 5, index_disk_manager);
    indexer = new BPlusTreeIndexer<Key, Compare>(index_bpm);
    data_disk_manager = new DiskManager(data_file_path);
    data_bpm = new BufferPoolManager(100, 5, data_disk_manager);
    data_storage = new SingleValueStorage<Value>(data_bpm);
  }
  ~DiskMap() {
    delete indexer;
    delete index_bpm;
    delete index_disk_manager;
    delete data_storage;
    delete data_bpm;
    delete data_disk_manager;
  }
  DiskMap(const DiskMap &) = delete;
  DiskMap &operator=(const DiskMap &) = delete;
  DiskMap(DiskMap &&) = delete;
  DiskMap &operator=(DiskMap &&) = delete;
  virtual sjtu::vector<FileEntry> ListFiles() override {
    sjtu::vector<FileEntry> res;
    res.push_back({index_file_identifier, index_file_path, index_disk_manager});
    res.push_back({data_file_identifier, data_file_path, data_disk_manager});
    return res;
  }
  virtual void LockDownForCheckOut() override {
    delete indexer;
    delete index_bpm;
    delete index_disk_manager;
    delete data_storage;
    delete data_bpm;
    delete data_disk_manager;
    indexer = nullptr;
    index_bpm = nullptr;
    index_disk_manager = nullptr;
    data_storage = nullptr;
    data_bpm = nullptr;
    data_disk_manager = nullptr;
  }
  bool HasKey(const Key &key) { return indexer->Get(key) != kInvalidValueIndex; }
  Value Get(const Key &key) {
    size_t data_id;
    if ((data_id = indexer->Get(key)) == kInvalidValueIndex) throw std::runtime_error("Key not found");
    Value res;
    data_storage->read(res, data_id);
    return res;
  }
  void Get(const Key &key, Value &res) {
    size_t data_id;
    if ((data_id = indexer->Get(key)) == kInvalidValueIndex) throw std::runtime_error("Key not found");
    data_storage->read(res, data_id);
  }
  size_t size() { return indexer->Size(); }
  bool Remove(const Key &key) {
    b_plus_tree_value_index_t data_id;
    bool remove_success = indexer->Remove(key, &data_id);
    if (!remove_success) return false;
    data_storage->Delete(data_id);
    return true;
  }
  bool Put(const Key &key, Value &value) {
    b_plus_tree_value_index_t data_id;
    data_id = indexer->Get(key);
    if (data_id != kInvalidValueIndex) {
      data_storage->update(value, data_id);
      return false;
    }
    data_id = data_storage->write(value);
    indexer->Put(key, data_id);
    return true;
  }
  virtual void Flush() override {
    if (indexer == nullptr) return;
    indexer->Flush();
    data_storage->Flush();
  }
};
#endif  // DISK_MAP_H