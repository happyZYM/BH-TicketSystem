#ifndef STOP_REGISTER_HPP
#define STOP_REGISTER_HPP
#include <cstdint>
#include <vector>
#include "basic_defs.h"
#include "data.h"
#include "storage/bpt.hpp"
#include "storage/buffer_pool_manager.h"
#include "storage/driver.h"
#include "utils.h"
struct stop_register_t {
  hash_t station_ID_hash;
  hash_t train_ID_hash;
  uint16_t type : 1;
  uint16_t startTime : 12;
  uint8_t stop_id;
};
inline bool operator<(const stop_register_t &A, const stop_register_t &B) {
  if (A.station_ID_hash != B.station_ID_hash) return A.station_ID_hash < B.station_ID_hash;
  if (A.train_ID_hash != B.train_ID_hash) return A.train_ID_hash < B.train_ID_hash;
  if (A.type != B.type) return A.type < B.type;
  return A.startTime < B.startTime;
}
struct MinimalTrainRecord {
  uint16_t saleDate_beg : 8, saleDate_end : 8, vis_time_offset : 14;
};
static_assert(sizeof(MinimalTrainRecord) == sizeof(default_numeric_index_t));

class StopRegister : public DataDriverBase {
  std::string bpt_file_identifier;
  std::string bpt_file_path;
  DiskManager *bpt_disk_manager;
  BufferPoolManager *bpt_bpm;
  BPlusTreeIndexer<stop_register_t, std::less<stop_register_t>> *bpt_indexer;

 public:
  struct DirectTrainInfo {
    hash_t train_ID_hash;
    int actual_start_date;
    int leave_time_stamp;
    int arrive_time_stamp;
    int from_stop_id;
    int to_stop_id;
    int saleDate_beg;
  };
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
                          uint16_t true_saleDate_end, uint16_t startTime, uint16_t arrive_time_offset,
                          uint16_t leave_time_offset, uint8_t stop_id) {
    LOG->debug(
        "AddStopInfo: station_hash {} train_hash {} true_saleDate_beg {} true_saleDate_end {} startTime {} "
        "arrive_time_offset {} leave_time_offset {} stop_id {}",
        station_hash, train_hash, true_saleDate_beg, true_saleDate_end, startTime, arrive_time_offset,
        leave_time_offset, stop_id);
    MinimalTrainRecord record_arrive, record_leave;
    const static int June_1st_2024 = 152;
    record_arrive.saleDate_beg = true_saleDate_beg - June_1st_2024;
    record_arrive.saleDate_end = true_saleDate_end - June_1st_2024;
    record_arrive.vis_time_offset = arrive_time_offset;
    record_leave.saleDate_beg = true_saleDate_beg - June_1st_2024;
    record_leave.saleDate_end = true_saleDate_end - June_1st_2024;
    record_leave.vis_time_offset = leave_time_offset;
    if (arrive_time_offset != uint16_t(-1))
      bpt_indexer->Put({station_hash, train_hash, 0, startTime, stop_id},
                       *reinterpret_cast<default_numeric_index_t *>(&record_arrive));
    if (leave_time_offset != uint16_t(-1))
      bpt_indexer->Put({station_hash, train_hash, 1, startTime, stop_id},
                       *reinterpret_cast<default_numeric_index_t *>(&record_leave));
  }
  inline void QueryDirectTrains(uint32_t date, hash_t from_station_ID, hash_t to_station_ID,
                                sjtu::vector<StopRegister::DirectTrainInfo> &res) {
    const static int June_1st_2024 = 152;
    auto it_from = bpt_indexer->lower_bound_const({from_station_ID, 0});
    auto it_to = bpt_indexer->lower_bound_const({to_station_ID, 0});
    while (it_from != bpt_indexer->end_const()) {
      const auto &key_from = it_from.GetKey();
      const auto &value_from = it_from.GetValue();
      LOG->debug("it_from now tries to check station_id_hash {} train_id_hash {} stop_id {}", key_from.station_ID_hash,
                 key_from.train_ID_hash, key_from.stop_id);
      if (key_from.station_ID_hash != from_station_ID) break;
      if (key_from.type != 1) {
        ++it_from;
        continue;
      }
      LOG->debug("it_from now checks station_id_hash {} train_id_hash {} stop_id {}", key_from.station_ID_hash,
                 key_from.train_ID_hash, key_from.stop_id);
      int true_saleDate_beg = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_beg + June_1st_2024;
      int true_saleDate_end = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_end + June_1st_2024;
      int leave_time_offset = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).vis_time_offset;
      int startTime = key_from.startTime;
      int actual_time = startTime + leave_time_offset;
      int delta_days = actual_time / 1440;
      if (date - delta_days < true_saleDate_beg || date - delta_days > true_saleDate_end) {
        ++it_from;
        continue;
      }
      StopRegister::DirectTrainInfo entry;
      entry.train_ID_hash = key_from.train_ID_hash;
      entry.actual_start_date = date - delta_days;
      entry.leave_time_stamp = entry.actual_start_date * 1440 + actual_time;
      entry.from_stop_id = key_from.stop_id;
      entry.saleDate_beg = true_saleDate_beg;
      while (it_to != bpt_indexer->end_const()) {
        const auto &key_to = it_to.GetKey();
        const auto &value_to = it_to.GetValue();
        if (key_to.station_ID_hash != to_station_ID) break;
        if (key_to.type != 0) {
          ++it_to;
          continue;
        }
        LOG->debug("it_to now checks station_id_hash {} train_id_hash {} stop_id {}", key_to.station_ID_hash,
                   key_to.train_ID_hash, key_to.stop_id);
        if (key_to.train_ID_hash > key_from.train_ID_hash) break;
        if (key_to.train_ID_hash == key_from.train_ID_hash) {
          entry.arrive_time_stamp = entry.actual_start_date * 1440 + startTime +
                                    (*reinterpret_cast<const MinimalTrainRecord *>(&value_to)).vis_time_offset;
          entry.to_stop_id = key_to.stop_id;
          // LOG->debug("leave_time_stamp {} arrive_time_stamp {}", entry.leave_time_stamp, entry.arrive_time_stamp);
          if (entry.leave_time_stamp < entry.arrive_time_stamp) res.push_back(entry);
          // res.push_back(entry);
          ++it_to;
          break;
        }
        ++it_to;
      }
      ++it_from;
    }
  }
  inline void RequestSingleTrain(hash_t train_ID_hash, int date, hash_t from_station_hash, hash_t to_station_hash,
                                 bool &success, StopRegister::DirectTrainInfo &entry) {
    const static int June_1st_2024 = 152;
    auto it_from = bpt_indexer->lower_bound_const({from_station_hash, train_ID_hash, 1});
    auto it_to = bpt_indexer->lower_bound_const({to_station_hash, train_ID_hash, 0});
    if (it_from == bpt_indexer->end_const() || it_to == bpt_indexer->end_const()) {
      success = false;
      return;
    }
    const auto &key_from = it_from.GetKey();
    const auto &key_to = it_to.GetKey();
    if (key_from.train_ID_hash != train_ID_hash || key_to.train_ID_hash != train_ID_hash ||
        key_from.station_ID_hash != from_station_hash || key_to.station_ID_hash != to_station_hash ||
        key_from.type != 1 || key_to.type != 0) {
      success = false;
      return;
    }
    const auto &value_from = it_from.GetValue();
    const auto &value_to = it_to.GetValue();
    if ((*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).vis_time_offset >
        (*reinterpret_cast<const MinimalTrainRecord *>(&value_to)).vis_time_offset) {
      success = false;
      return;
    }
    int true_saleDate_beg = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_beg + June_1st_2024;
    int true_saleDate_end = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_end + June_1st_2024;
    int leave_time_offset = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).vis_time_offset;
    int startTime = key_from.startTime;
    int actual_time = startTime + leave_time_offset;
    int delta_days = actual_time / 1440;
    if (date - delta_days < true_saleDate_beg || date - delta_days > true_saleDate_end) {
      success = false;
      return;
    }
    entry.train_ID_hash = key_from.train_ID_hash;
    entry.actual_start_date = date - delta_days;
    entry.leave_time_stamp = entry.actual_start_date * 1440 + actual_time;
    entry.from_stop_id = key_from.stop_id;
    entry.saleDate_beg = true_saleDate_beg;
    entry.arrive_time_stamp = entry.actual_start_date * 1440 + startTime +
                              (*reinterpret_cast<const MinimalTrainRecord *>(&value_to)).vis_time_offset;
    entry.to_stop_id = key_to.stop_id;
    success = true;
  }
  inline void FetchTrainLeavingFrom(uint32_t date, hash_t from_station_ID, sjtu::vector<hash_t> &res) {
    const static int June_1st_2024 = 152;
    res.clear();
    auto it_from = bpt_indexer->lower_bound_const({from_station_ID, 0});
    while (it_from != bpt_indexer->end_const()) {
      const auto &key_from = it_from.GetKey();
      const auto &value_from = it_from.GetValue();
      LOG->debug("it_from now tries to check station_id_hash {} train_id_hash {} stop_id {}", key_from.station_ID_hash,
                 key_from.train_ID_hash, key_from.stop_id);
      if (key_from.station_ID_hash != from_station_ID) break;
      if (key_from.type != 1) {
        ++it_from;
        continue;
      }
      LOG->debug("it_from now checks station_id_hash {} train_id_hash {} stop_id {}", key_from.station_ID_hash,
                 key_from.train_ID_hash, key_from.stop_id);
      int true_saleDate_beg = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_beg + June_1st_2024;
      int true_saleDate_end = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).saleDate_end + June_1st_2024;
      int leave_time_offset = (*reinterpret_cast<const MinimalTrainRecord *>(&value_from)).vis_time_offset;
      int startTime = key_from.startTime;
      int actual_time = startTime + leave_time_offset;
      int delta_days = actual_time / 1440;
      if (date - delta_days < true_saleDate_beg || date - delta_days > true_saleDate_end) {
        ++it_from;
        continue;
      }
      res.push_back(key_from.train_ID_hash);
      ++it_from;
    }
  }
  inline void FetchTrainArriavingAt(uint32_t date, hash_t to_station_ID, sjtu::vector<hash_t> &res) {
    res.clear();
    auto it_to = bpt_indexer->lower_bound_const({to_station_ID, 0});
    while (it_to != bpt_indexer->end_const()) {
      const auto &key_to = it_to.GetKey();
      LOG->debug("it_to now tries to check station_id_hash {} train_id_hash {} stop_id {}", key_to.station_ID_hash,
                 key_to.train_ID_hash, key_to.stop_id);
      if (key_to.station_ID_hash != to_station_ID) break;
      if (key_to.type != 0) {
        ++it_to;
        continue;
      }
      res.push_back(key_to.train_ID_hash);
      ++it_to;
    }
  }
};

#endif