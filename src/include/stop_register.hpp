#ifndef STOP_REGISTER_HPP
#define STOP_REGISTER_HPP
#include <vector>
#include "basic_defs.h"
#include "data.h"
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/driver.h"
typedef std::pair<hash_t, hash_t> stop_register_t;  // The first is station hash, the second is train hash
struct MinimalTrainRecord {
  uint16_t saleDate_beg : 7, saleDate_end : 7, vis_time_offset : 13, type : 1;
};
static_assert(sizeof(MinimalTrainRecord) == sizeof(default_numeric_index_t));

class StopRegister : public DataDriverBase {
  std::string bpt_file_identifier;
  std::string bpt_file_path;
  DiskManager *bpt_disk_manager;
  BufferPoolManager *bpt_bpm;
  BPlusTreeIndexer<stop_register_t, std::less<stop_register_t>> *bpt_indexer;

 public:
  // for satety, all the copy/move operations are deleted, please manage it using pointer
  StopRegister &operator=(const StopRegister &) = delete;
  StopRegister(const StopRegister &) = delete;
  StopRegister &operator=(StopRegister &&) = delete;
  StopRegister(StopRegister &&) = delete;
  inline StopRegister(std::string bpt_file_identifier_, std::string bpt_file_path_)
      : bpt_file_identifier(std::move(bpt_file_identifier_)), bpt_file_path(std::move(bpt_file_path_)) {
    bpt_disk_manager = new DiskManager(bpt_file_path);
    bpt_bpm = new BufferPoolManager(100, 5, bpt_disk_manager);
    bpt_indexer = new BPlusTreeIndexer<stop_register_t, std::less<stop_register_t>>(bpt_bpm);
  }
  inline ~StopRegister() {
    delete bpt_indexer;
    delete bpt_bpm;
    delete bpt_disk_manager;
  }
  inline virtual sjtu::vector<FileEntry> ListFiles() override {
    sjtu::vector<FileEntry> res;
    res.push_back({bpt_file_identifier, bpt_file_path, bpt_disk_manager});
    return res;
  }
  inline virtual void LockDownForCheckOut() override {
    delete bpt_indexer;
    delete bpt_bpm;
    delete bpt_disk_manager;
    bpt_indexer = nullptr;
    bpt_bpm = nullptr;
    bpt_disk_manager = nullptr;
  }
  inline virtual void Flush() override {
    if (bpt_indexer == nullptr) return;
    bpt_indexer->Flush();
  }
  inline void AddStopInfo(hash_t station_hash, hash_t train_hash, uint16_t true_saleDate_beg,
                          uint16_t true_saleDate_end, uint16_t arrive_time_offset, uint16_t leave_time_offset) {
    MinimalTrainRecord record_arrive, record_leave;
    const static int June_1st_2024 = 152;
    record_arrive.saleDate_beg = true_saleDate_beg - June_1st_2024;
    record_arrive.saleDate_end = true_saleDate_end - June_1st_2024;
    record_arrive.vis_time_offset = arrive_time_offset;
    record_arrive.type = 0;
    record_leave.saleDate_beg = true_saleDate_beg - June_1st_2024;
    record_leave.saleDate_end = true_saleDate_end - June_1st_2024;
    record_leave.vis_time_offset = leave_time_offset;
    record_leave.type = 1;
    if (arrive_time_offset != uint16_t(-1))
      bpt_indexer->Put({station_hash, train_hash}, *reinterpret_cast<default_numeric_index_t *>(&record_arrive));
    if (leave_time_offset != uint16_t(-1))
      bpt_indexer->Put({station_hash, train_hash}, *reinterpret_cast<default_numeric_index_t *>(&record_leave));
  }
  inline void GetShareItems(std::vector<hash_t> &A, std::vector<hash_t> &B, std::vector<hash_t> &res) {
    // TODO
  }
  inline void QueryDirectTrains(uint32_t date, hash_t from_station_ID, hash_t to_station_ID, std::vector<hash_t> &res) {
    std::vector<hash_t> valid_trains_pass_from, valid_trains_pass_to;
    res.clear();
    // TODO
    GetShareItems(valid_trains_pass_from, valid_trains_pass_to, res);
  }
};

#endif