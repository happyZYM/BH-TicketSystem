#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "engine.h"
#include "utils.h"

std::string TicketSystemEngine::AddTrain(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string trainID;
  int stationNum, seatNum;
  std::string stations[100];
  int prices[100];
  int startTime;
  int travelTimes[100], stopoverTimes[100];
  int saleDate_begin, saleDate_end;
  char type[2];
  command_stream >> token >> token;
  while (command_stream >> token) {
    LOG->debug("token: {}", token);
    switch (token[1]) {
      case 'i': {
        command_stream >> trainID;
        break;
      }
      case 'n': {
        command_stream >> stationNum;
        break;
      }
      case 'm': {
        command_stream >> seatNum;
        break;
      }
      case 's': {
        std::string station_raw;
        command_stream >> station_raw;
        std::stringstream station_raw_stream(station_raw);
        // the sperator is '|' in the raw string
        for (int i = 0; i < stationNum; i++) {
          std::getline(station_raw_stream, stations[i], '|');
        }
        break;
      }
      case 'p': {
        std::string price_raw;
        command_stream >> price_raw;
        std::stringstream price_raw_stream(price_raw);
        for (int i = 0; i < stationNum - 1; i++) {
          std::string tmp;
          std::getline(price_raw_stream, tmp, '|');
          sscanf(tmp.c_str(), "%d", &prices[i]);
        }
        break;
      }
      case 'x': {
        std::string startTime_raw;
        command_stream >> startTime_raw;
        int hh, mm;
        sscanf(startTime_raw.c_str(), "%d:%d", &hh, &mm);
        startTime = hh * 60 + mm;
        break;
      }
      case 't': {
        std::string travelTime_raw;
        command_stream >> travelTime_raw;
        std::stringstream travelTime_raw_stream(travelTime_raw);
        for (int i = 0; i < stationNum - 1; i++) {
          std::string tmp;
          std::getline(travelTime_raw_stream, tmp, '|');
          sscanf(tmp.c_str(), "%d", &travelTimes[i]);
        }
        break;
      }
      case 'o': {
        std::string stopoverTime_raw;
        command_stream >> stopoverTime_raw;
        std::stringstream stopoverTime_raw_stream(stopoverTime_raw);
        for (int i = 1; i < stationNum - 1; i++) {
          std::string tmp;
          std::getline(stopoverTime_raw_stream, tmp);
          sscanf(tmp.c_str(), "%d", &stopoverTimes[i]);
        }
        break;
      }
      case 'd': {
        std::string saleDate_raw;
        command_stream >> saleDate_raw;
        int beg_mm, beg_dd, end_mm, end_dd;
        sscanf(saleDate_raw.c_str(), "%d-%d|%d-%d", &beg_mm, &beg_dd, &end_mm, &end_dd);
        saleDate_begin = GetCompactDate(beg_mm, beg_dd);
        saleDate_end = GetCompactDate(end_mm, end_dd);
        break;
      }
      case 'y': {
        command_stream >> type;
        break;
      }
      default: {
        throw std::runtime_error("fatal error: unknown token");
      }
    }
  }
  LOG->debug("trainID: {}", trainID);
  LOG->debug("stationNum: {}", stationNum);
  LOG->debug("seatNum: {}", seatNum);
  LOG->debug("stations:");
  for (int i = 0; i < stationNum; i++) {
    LOG->debug("{} {}", i, stations[i]);
  }
  LOG->debug("prices:");
  for (int i = 0; i < stationNum - 1; i++) {
    LOG->debug("{} {}", i, prices[i]);
  }
  LOG->debug("startTime: {}={}:{}", startTime, startTime / 60, startTime % 60);
  LOG->debug("travelTimes:");
  for (int i = 0; i < stationNum - 1; i++) {
    LOG->debug("{} {}", i, travelTimes[i]);
  }
  LOG->debug("stopoverTimes:");
  for (int i = 1; i < stationNum - 1; i++) {
    LOG->debug("{} {}", i, stopoverTimes[i]);
  }
  LOG->debug("saleDate: from {}={}-{} to {}={}-{}", saleDate_begin, RetrieveReadableDate(saleDate_begin).first,
             RetrieveReadableDate(saleDate_begin).second, saleDate_end, RetrieveReadableDate(saleDate_end).first,
             RetrieveReadableDate(saleDate_end).second);
  LOG->debug("type: {}", type);
  hash_t train_id_hash = SplitMix64Hash(trainID);
  if (ticket_price_data_storage.HasKey(train_id_hash)) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  TicketPriceData ticket_price_data;
  for (int i = 0; i < stationNum - 1; i++) ticket_price_data.price[i] = prices[i];
  ticket_price_data_storage.Put(train_id_hash, ticket_price_data);
  CoreTrainData core_train_data;
  core_train_data.is_released = 0;
  strcpy(core_train_data.trainID, trainID.c_str());
  core_train_data.stationNum = stationNum;
  for (int i = 0; i < stationNum; i++) {
    core_train_data.stations_hash[i] = SplitMix64Hash(stations[i]);
  }
  core_train_data.seatNum = seatNum;
  core_train_data.startTime = startTime;
  for (int i = 0; i < stationNum - 1; i++) {
    core_train_data.travelTime[i] = travelTimes[i];
    core_train_data.stopoverTime[i] = stopoverTimes[i];
  }
  core_train_data.saleDate_beg = saleDate_begin;
  core_train_data.saleDate_end = saleDate_end;
  core_train_data.type = type[0] - 'A';
  core_train_data_storage.Put(train_id_hash, core_train_data);
  StationNameData station_name_data;
  for (int i = 0; i < stationNum; i++) {
    size_t len = stations[i].length();
    for (int j = 0; j < len; j++) station_name_data.name[i][j] = stations[i][j];
    if (len < 40) station_name_data.name[i][len] = '\0';
  }
  station_name_data_storage.Put(train_id_hash, station_name_data);
  response_stream << '[' << command_id << "] 0";
  return response_stream.str();
}

std::string TicketSystemEngine::DeleteTrain(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string trainID;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'i': {
        command_stream >> trainID;
        break;
      }
    }
  }
  // TODO
  response_stream << '[' << command_id << "] DeleteTrain";
  return response_stream.str();
}

std::string TicketSystemEngine::ReleaseTrain(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string trainID;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'i': {
        command_stream >> trainID;
        break;
      }
    }
  }
  // TODO
  response_stream << '[' << command_id << "] ReleaseTrain";
  return response_stream.str();
}

std::string TicketSystemEngine::QueryTrain(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string trainID;
  int date;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'i': {
        command_stream >> trainID;
        break;
      }
      case 'd': {
        std::string date_raw;
        command_stream >> date_raw;
        int mm, dd;
        sscanf(date_raw.c_str(), "%d-%d", &mm, &dd);
        date = GetCompactDate(mm, dd);
        break;
      }
    }
  }
  LOG->debug("trainID: {}", trainID);
  LOG->debug("date: {}={}-{}", date, RetrieveReadableDate(date).first, RetrieveReadableDate(date).second);
  // TODO
  response_stream << '[' << command_id << "] QueryTrain";
  return response_stream.str();
}