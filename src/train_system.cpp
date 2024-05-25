#include <cstring>
#include <iomanip>
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
        for (int i = 0; std::getline(station_raw_stream, stations[i], '|'); i++)
          ;
        break;
      }
      case 'p': {
        std::string price_raw;
        command_stream >> price_raw;
        std::stringstream price_raw_stream(price_raw);
        std::string tmp;
        for (int i = 0; std::getline(price_raw_stream, tmp, '|'); i++) {
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
        LOG->debug("travelTime_raw: {}", travelTime_raw);
        std::string tmp;
        for (int i = 0; std::getline(travelTime_raw_stream, tmp, '|'); i++) {
          sscanf(tmp.c_str(), "%d", &travelTimes[i]);
          LOG->debug("i={} tmp={} travelTimes[{}]={}", i, tmp, i, travelTimes[i]);
        }
        break;
      }
      case 'o': {
        std::string stopoverTime_raw;
        command_stream >> stopoverTime_raw;
        std::stringstream stopoverTime_raw_stream(stopoverTime_raw);
        std::string tmp;
        for (int i = 1; std::getline(stopoverTime_raw_stream, tmp, '|'); i++) {
          sscanf(tmp.c_str(), "%d", &stopoverTimes[i]);
        }
        break;
      }
      case 'd': {
        std::string saleDate_raw;
        command_stream >> saleDate_raw;
        int beg_mm, beg_dd, end_mm, end_dd;
        sscanf(saleDate_raw.c_str(), "%d-%d|%d-%d", &beg_mm, &beg_dd, &end_mm, &end_dd);
        if (beg_mm < 6 || end_mm > 8) throw std::runtime_error("fatal error: sale date out of range");
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
  strcpy(ticket_price_data.trainID, trainID.c_str());
  ticket_price_data_storage.Put(train_id_hash, ticket_price_data);
  CoreTrainData core_train_data;
  core_train_data.is_released = 0;
  strcpy(core_train_data.trainID, trainID.c_str());
  core_train_data.stationNum = stationNum;
  for (int i = 0; i < stationNum; i++) {
    core_train_data.stations_hash[i] = SplitMix64Hash(stations[i]);
    LOG->debug("set core_train_data.stations_hash[{}]={}", i, core_train_data.stations_hash[i]);
  }
  core_train_data.seatNum = seatNum;
  core_train_data.startTime = startTime;
  for (int i = 0; i < stationNum - 1; i++) {
    core_train_data.travelTime[i] = travelTimes[i];
    core_train_data.stopoverTime[i] = stopoverTimes[i];
    LOG->debug("set core_train_data.travelTime[{}]={}", i, core_train_data.travelTime[i]);
    LOG->debug("set core_train_data.stopoverTime[{}]={}", i, core_train_data.stopoverTime[i]);
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
  SeatsData seats_data;
  for (int i = 0; i < core_train_data.stationNum - 1; i++) {
    seats_data.seat[i] = core_train_data.seatNum;
  }
  seats_data.max_seats = core_train_data.seatNum;
  int day_count = core_train_data.saleDate_end - core_train_data.saleDate_beg + 1;
  for (int i = 0; i < day_count; i++) {
    seats_data_storage.Put(std::make_pair(train_id_hash, i), seats_data);
  }
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
  hash_t train_id_hash = SplitMix64Hash(trainID);
  CoreTrainData core_train_data;
  try {
    core_train_data_storage.Get(train_id_hash, core_train_data);
    if (core_train_data.is_released == 1) {
      response_stream << '[' << command_id << "] -1";
      return response_stream.str();
    }
  } catch (std::runtime_error &e) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  core_train_data_storage.Remove(train_id_hash);
  ticket_price_data_storage.Remove(train_id_hash);
  station_name_data_storage.Remove(train_id_hash);
  int day_count = core_train_data.saleDate_end - core_train_data.saleDate_beg + 1;
  for (int i = 0; i < day_count; i++) {
    seats_data_storage.Remove(std::make_pair(train_id_hash, i));
  }
  response_stream << '[' << command_id << "] 0";
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
  hash_t train_id_hash = SplitMix64Hash(trainID);
  LOG->debug("hash({})={}", trainID, train_id_hash);
  CoreTrainData core_train_data;
  try {
    core_train_data_storage.Get(train_id_hash, core_train_data);
    if (core_train_data.is_released == 1) {
      response_stream << '[' << command_id << "] -1";
      return response_stream.str();
    }
  } catch (std::runtime_error &e) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  core_train_data.is_released = 1;
  core_train_data_storage.Put(train_id_hash, core_train_data);
  // TODO: update data to transaction system
  int vis_time_offset = 0;
  for (int i = 0; i < core_train_data.stationNum; i++) {
    uint16_t arrive_time_offset = -1;
    uint16_t leave_time_offset = -1;
    LOG->debug("i={}", i);
    if (i != 0) {
      vis_time_offset += core_train_data.travelTime[i - 1];
      LOG->debug("vis_time_offset += travelTime[{}]={}", i - 1, core_train_data.travelTime[i - 1]);
      arrive_time_offset = vis_time_offset;
      LOG->debug("set arrive_time_offset={}", arrive_time_offset);
    }
    if (i != core_train_data.stationNum - 1) {
      if (i != 0) {
        vis_time_offset += core_train_data.stopoverTime[i];
        LOG->debug("vis_time_offset += stopoverTime[{}]={}", i, core_train_data.stopoverTime[i]);
      }
      leave_time_offset = vis_time_offset;
      LOG->debug("set leave_time_offset={}", leave_time_offset);
    }
    stop_register.AddStopInfo(core_train_data.stations_hash[i], train_id_hash, core_train_data.saleDate_beg,
                              core_train_data.saleDate_end, core_train_data.startTime, arrive_time_offset,
                              leave_time_offset, i);
  }
  transaction_manager.PrepareTrainInfo(train_id_hash, core_train_data.saleDate_end - core_train_data.saleDate_beg + 1);
  response_stream << '[' << command_id << "] 0";
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
  hash_t train_id_hash = SplitMix64Hash(trainID);
  CoreTrainData core_train_data;
  try {
    core_train_data_storage.Get(train_id_hash, core_train_data);
  } catch (std::runtime_error &e) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  if (date < core_train_data.saleDate_beg || date > core_train_data.saleDate_end) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  StationNameData station_name_data;
  station_name_data_storage.Get(train_id_hash, station_name_data);
  LOG->debug("successfully retrieved station name data");
  TicketPriceData ticket_price_data;
  ticket_price_data_storage.Get(train_id_hash, ticket_price_data);
  LOG->debug("successfully retrieved ticket price data");
  SeatsData seats_data;
  seats_data_storage.Get(std::make_pair(train_id_hash, date - core_train_data.saleDate_beg), seats_data);
  LOG->debug("successfully retrieved seats data");
  response_stream << '[' << command_id << "] " << trainID << ' ' << char(core_train_data.type + 'A') << '\n';
  int cur_time = date * 1440 + core_train_data.startTime;
  int total_price = 0;
  for (int i = 0; i < core_train_data.stationNum; i++) {
    for (int j = 0; j < 40 && station_name_data.name[i][j] != '\0'; j++)
      response_stream << station_name_data.name[i][j];
    if (i == 0) {
      response_stream << " xx-xx xx:xx -> ";
      int month, day, hour, minute;
      RetrieveReadableTimeStamp(cur_time, month, day, hour, minute);
      response_stream << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0') << day
                      << ' ' << std::setw(2) << std::setfill('0') << hour << ':' << std::setw(2) << std::setfill('0')
                      << minute << ' ';
      response_stream << total_price << ' ' << seats_data.seat[i];
    } else if (i < core_train_data.stationNum - 1) {
      int month, day, hour, minute;
      RetrieveReadableTimeStamp(cur_time, month, day, hour, minute);
      response_stream << ' ' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0')
                      << day << ' ' << std::setw(2) << std::setfill('0') << hour << ':' << std::setw(2)
                      << std::setfill('0') << minute << " -> ";
      cur_time += core_train_data.stopoverTime[i];
      RetrieveReadableTimeStamp(cur_time, month, day, hour, minute);
      response_stream << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0') << day
                      << ' ' << std::setw(2) << std::setfill('0') << hour << ':' << std::setw(2) << std::setfill('0')
                      << minute << ' ';
      response_stream << total_price << ' ' << seats_data.seat[i];
    } else {
      int month, day, hour, minute;
      RetrieveReadableTimeStamp(cur_time, month, day, hour, minute);
      response_stream << ' ' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0')
                      << day << ' ' << std::setw(2) << std::setfill('0') << hour << ':' << std::setw(2)
                      << std::setfill('0') << minute << " -> xx-xx xx:xx ";
      response_stream << total_price << " x";
    }
    if (i != core_train_data.stationNum - 1) {
      total_price += ticket_price_data.price[i];
      response_stream << '\n';
    }
    cur_time += core_train_data.travelTime[i];
  }
  return response_stream.str();
}