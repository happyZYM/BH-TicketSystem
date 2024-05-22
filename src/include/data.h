#ifndef DATA_H
#define DATA_H
#include <cstdint>
#include "storage/driver.h"
#include "utils.h"
struct FullUserData {
  char username[21];
  hash_t password_hash;
  char name[21];
  char mailAddr[31];
  uint8_t privilege;
};

// waring: this struct is extremely large, later DiskManager should be optimized to handle this
struct FullTrainData {
  char trainID[21];
  uint8_t stationNum;
  // char stations[100][41];
  hash_t stations_hash[100];
  uint32_t seatNum : 18;
  uint16_t startTime : 12;
  uint16_t saleDate_beg : 10, saleDate_end : 10;
  uint8_t type : 6;
  struct StationData {
    uint32_t price : 18;
    uint16_t travelTime : 15;
    uint16_t stopoverTime : 15;
  };
  StationData stations[100];
};

class TrainDataDrive: public DataDriverBase {

};
#endif