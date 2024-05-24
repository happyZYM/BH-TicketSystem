#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "engine.h"
#include "utils.h"

std::string TicketSystemEngine::QueryTicket(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  int date;
  std::string from, to;
  std::string order_by = "time";
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'd': {
        std::string date_raw;
        command_stream >> date_raw;
        int mm, dd;
        sscanf(date_raw.c_str(), "%d-%d", &mm, &dd);
        date = GetCompactDate(mm, dd);
        break;
      }
      case 's': {
        command_stream >> from;
        break;
      }
      case 't': {
        command_stream >> to;
        break;
      }
      case 'p': {
        command_stream >> order_by;
        break;
      }
      default: {
        throw std::invalid_argument("Invalid argument");
      }
    }
  }
  LOG->debug("date {}={}-{}, from {}, to {}, order by {}", date, RetrieveReadableDate(date).first,
             RetrieveReadableDate(date).second, from, to, order_by);
  // TODO
  hash_t from_hash = SplitMix64Hash(from), to_hash = SplitMix64Hash(to);
  std::vector<StopRegister::DirectTrainInfo> valid_trains;
  stop_register.QueryDirectTrains(date, from_hash, to_hash, valid_trains);
  size_t len = valid_trains.size();
  std::vector<std::pair<StopRegister::DirectTrainInfo, AdditionalTrainInfo>> valid_trains_full(len);
  LOG->debug("retrieving full data");
  for (size_t i = 0; i < len; i++) {
    valid_trains_full[i].first = valid_trains[i];
    TicketPriceData ticket_price_data;
    SeatsData seats_data;
    int from_station_id = valid_trains[i].from_stop_id;
    int to_station_id = valid_trains[i].to_stop_id;
    ticket_price_data_storage.Get(valid_trains[i].train_ID_hash, ticket_price_data);
    LOG->debug("analyzing train {} from {} to {}", ticket_price_data.trainID, from_station_id, to_station_id);
    seats_data_storage.Get(
        {valid_trains[i].train_ID_hash, valid_trains[i].actual_start_date - valid_trains[i].saleDate_beg}, seats_data);
    strcpy(valid_trains_full[i].second.trainID, ticket_price_data.trainID);
    int total_price = 0;
    for (int j = from_station_id; j < to_station_id; j++) {
      total_price += ticket_price_data.price[j];
    }
    valid_trains_full[i].second.price = total_price;
    int seats = seats_data.seat[from_station_id];
    for (int j = from_station_id + 1; j < to_station_id; j++) {
      seats = std::min(seats, (int)seats_data.seat[j]);
    }
    valid_trains_full[i].second.seats = seats;
  }
  LOG->debug("successfully retrieved full data");
  std::vector<int> valid_trains_full_index(len);
  for (size_t i = 0; i < len; i++) {
    valid_trains_full_index[i] = i;
  }
  if (order_by == "time") {
    auto cmp = [&valid_trains_full](int a, int b) {
      int time_cost_a = valid_trains_full[a].first.arrive_time_stamp - valid_trains_full[a].first.leave_time_stamp;
      int time_cost_b = valid_trains_full[b].first.arrive_time_stamp - valid_trains_full[b].first.leave_time_stamp;
      if (time_cost_a != time_cost_b) return time_cost_a < time_cost_b;
      return strcmp(valid_trains_full[a].second.trainID, valid_trains_full[b].second.trainID) < 0;
    };
    std::sort(valid_trains_full_index.begin(), valid_trains_full_index.end(), cmp);
  } else {
    // order by price
    auto cmp = [&valid_trains_full](int a, int b) {
      if (valid_trains_full[a].second.price != valid_trains_full[b].second.price)
        return valid_trains_full[a].second.price < valid_trains_full[b].second.price;
      return strcmp(valid_trains_full[a].second.trainID, valid_trains_full[b].second.trainID) < 0;
    };
    std::sort(valid_trains_full_index.begin(), valid_trains_full_index.end(), cmp);
  }
  response_stream << "[" << command_id << "] " << len;
  for (int i = 0; i < len; i++) {
    response_stream << '\n';
    response_stream << valid_trains_full[valid_trains_full_index[i]].second.trainID << ' ' << from << ' ';
    int leave_time_stamp = valid_trains_full[valid_trains_full_index[i]].first.leave_time_stamp;
    int leave_time_month, leave_time_day, leave_time_hour, leave_time_minute;
    RetrieveReadableTimeStamp(leave_time_stamp, leave_time_month, leave_time_day, leave_time_hour, leave_time_minute);
    response_stream << std::setw(2) << std::setfill('0') << leave_time_month << '-' << std::setw(2) << std::setfill('0')
                    << leave_time_day << ' ' << std::setw(2) << std::setfill('0') << leave_time_hour << ':'
                    << std::setw(2) << std::setfill('0') << leave_time_minute;
    response_stream << " -> " << to << ' ';
    int arrive_time_stamp = valid_trains_full[valid_trains_full_index[i]].first.arrive_time_stamp;
    int arrive_time_month, arrive_time_day, arrive_time_hour, arrive_time_minute;
    RetrieveReadableTimeStamp(arrive_time_stamp, arrive_time_month, arrive_time_day, arrive_time_hour,
                              arrive_time_minute);
    response_stream << std::setw(2) << std::setfill('0') << arrive_time_month << '-' << std::setw(2)
                    << std::setfill('0') << arrive_time_day << ' ' << std::setw(2) << std::setfill('0')
                    << arrive_time_hour << ':' << std::setw(2) << std::setfill('0') << arrive_time_minute;
    response_stream << ' ' << valid_trains_full[valid_trains_full_index[i]].second.price << ' '
                    << valid_trains_full[valid_trains_full_index[i]].second.seats;
  }
  return response_stream.str();
}

std::string TicketSystemEngine::QueryTransfer(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  int date;
  std::string from, to;
  std::string order_by = "time";
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'd': {
        std::string date_raw;
        command_stream >> date_raw;
        int mm, dd;
        sscanf(date_raw.c_str(), "%d-%d", &mm, &dd);
        date = GetCompactDate(mm, dd);
        break;
      }
      case 's': {
        command_stream >> from;
        break;
      }
      case 't': {
        command_stream >> to;
        break;
      }
      case 'p': {
        command_stream >> order_by;
        break;
      }
      default: {
        throw std::invalid_argument("Invalid argument");
      }
    }
  }
  LOG->debug("date {}={}-{}, from {}, to {}, order by {}", date, RetrieveReadableDate(date).first,
             RetrieveReadableDate(date).second, from, to, order_by);
  // TODO
  response_stream << "[" << command_id << "] QueryTransfer";
  return response_stream.str();
}

std::string TicketSystemEngine::BuyTicket(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string user_name;
  std::string train_id;
  int date;
  std::string from, to;
  int ticket_num;
  std::string accept_queue = "false";
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'u': {
        command_stream >> user_name;
        break;
      }
      case 'i': {
        command_stream >> train_id;
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
      case 'f': {
        command_stream >> from;
        break;
      }
      case 't': {
        command_stream >> to;
        break;
      }
      case 'n': {
        command_stream >> ticket_num;
        break;
      }
      case 'q': {
        command_stream >> accept_queue;
        break;
      }
    }
  }
  LOG->debug("user {}, train {}, date {}={}-{}, from {}, to {}, ticket num {}, accept queue {}", user_name, train_id,
             date, RetrieveReadableDate(date).first, RetrieveReadableDate(date).second, from, to, ticket_num,
             accept_queue);
  // TODO
  response_stream << "[" << command_id << "] BuyTicket";
  return response_stream.str();
}

std::string TicketSystemEngine::QueryOrder(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string user_name;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'u': {
        command_stream >> user_name;
        break;
      }
    }
  }
  response_stream << "[" << command_id << "] QueryOrder";
  return response_stream.str();
}

std::string TicketSystemEngine::RefundTicket(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token;
  std::string user_name;
  int order = 1;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'u': {
        command_stream >> user_name;
        break;
      }
      case 'n': {
        command_stream >> order;
        break;
      }
    }
  }
  response_stream << "[" << command_id << "] RefundTicket";
  return response_stream.str();
}