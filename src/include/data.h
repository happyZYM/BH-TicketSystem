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

struct StationNameData {
  char name[100][40];
};
static_assert(sizeof(StationNameData) == 4000);
struct TicketPriceData {
  char trainID[21];
  uint32_t price[99];
};

struct AdditionalTrainInfo {
  char trainID[21];
  int price;
  int seats;
};

struct CoreTrainData {
  char trainID[21];
  uint8_t stationNum;
  hash_t stations_hash[100];
  uint32_t seatNum : 18;
  uint16_t startTime : 12;
  uint16_t saleDate_beg : 10, saleDate_end : 10;
  uint8_t type : 6;
  uint8_t is_released : 1;
  uint16_t travelTime[100];
  uint16_t stopoverTime[100];
};

struct SeatsData {
  uint32_t seat[99];
};
#endif