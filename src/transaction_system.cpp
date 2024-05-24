#include <algorithm>
#include <cstddef>
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
  hash_t user_ID_hash = SplitMix64Hash(user_name);
  hash_t train_ID_hash = SplitMix64Hash(train_id);
  if (online_users.find(user_ID_hash) == online_users.end()) {
    LOG->debug("user {} not online", user_name);
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  bool success = false;
  StopRegister::DirectTrainInfo info;
  hash_t from_station_hash = SplitMix64Hash(from), to_station_hash = SplitMix64Hash(to);
  stop_register.RequestSingleTrain(train_ID_hash, date, from_station_hash, to_station_hash, success, info);
  if (!success) {
    LOG->debug("no train available");
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  TicketPriceData ticket_price_data;
  SeatsData seats_data;
  int from_station_id = info.from_stop_id;
  int to_station_id = info.to_stop_id;
  int total_price = 0;
  int available_seats = 0;
  ticket_price_data_storage.Get(train_ID_hash, ticket_price_data);
  seats_data_storage.Get({train_ID_hash, info.actual_start_date - info.saleDate_beg}, seats_data);
  for (int j = from_station_id; j < to_station_id; j++) {
    total_price += ticket_price_data.price[j];
  }
  available_seats = seats_data.seat[from_station_id];
  for (int j = from_station_id + 1; j < to_station_id; j++) {
    available_seats = std::min(available_seats, (int)seats_data.seat[j]);
  }
  if (ticket_num > available_seats) {
    if (accept_queue == "false") {
      LOG->debug("no enough seats");
      response_stream << "[" << command_id << "] -1";
      return response_stream.str();
    }
    transaction_manager.AddOrder(train_id, from, to, 0, info.leave_time_stamp, info.arrive_time_stamp, ticket_num,
                                 total_price * (unsigned long long)ticket_num,
                                 info.actual_start_date - info.saleDate_beg, user_name, info.from_stop_id,
                                 info.to_stop_id);
    response_stream << "[" << command_id << "] queue";
    return response_stream.str();
  }
  transaction_manager.AddOrder(train_id, from, to, 1, info.leave_time_stamp, info.arrive_time_stamp, ticket_num,
                               total_price * (unsigned long long)ticket_num, info.actual_start_date - info.saleDate_beg,
                               user_name, info.from_stop_id, info.to_stop_id);
  for (int j = from_station_id; j < to_station_id; j++) {
    seats_data.seat[j] -= ticket_num;
  }
  seats_data_storage.Put({train_ID_hash, info.actual_start_date - info.saleDate_beg}, seats_data);
  response_stream << "[" << command_id << "] " << total_price * (unsigned long long)ticket_num;
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
  hash_t user_ID_hash = SplitMix64Hash(user_name);
  if (online_users.find(user_ID_hash) == online_users.end()) {
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  std::vector<b_plus_tree_value_index_t> his_idxs;
  transaction_manager.FetchFullUserOrderHistory(user_ID_hash, his_idxs);
  size_t len = his_idxs.size();
  TransactionData txn_data;
  response_stream << "[" << command_id << "] " << len;
  for (size_t i = 0; i < len; i++) {
    transaction_manager.FetchTransactionData(his_idxs[i], txn_data);
    response_stream << "\n[";
    if (txn_data.status == 0) {
      response_stream << "pending] ";
    } else if (txn_data.status == 1) {
      response_stream << "success] ";
    } else {
      response_stream << "refunded] ";
    }
    response_stream << txn_data.trainID << " " << txn_data.from_station_name << " ";
    int leave_month, leave_day, leave_hour, leave_minute;
    RetrieveReadableTimeStamp(txn_data.leave_time_stamp, leave_month, leave_day, leave_hour, leave_minute);
    response_stream << std::setw(2) << std::setfill('0') << leave_month << '-' << std::setw(2) << std::setfill('0')
                    << leave_day << ' ' << std::setw(2) << std::setfill('0') << leave_hour << ':' << std::setw(2)
                    << std::setfill('0') << leave_minute;
    response_stream << " -> " << txn_data.to_station_name << " ";
    int arrive_month, arrive_day, arrive_hour, arrive_minute;
    RetrieveReadableTimeStamp(txn_data.arrive_time_stamp, arrive_month, arrive_day, arrive_hour, arrive_minute);
    response_stream << std::setw(2) << std::setfill('0') << arrive_month << '-' << std::setw(2) << std::setfill('0')
                    << arrive_day << ' ' << std::setw(2) << std::setfill('0') << arrive_hour << ':' << std::setw(2)
                    << std::setfill('0') << arrive_minute;
    response_stream << " " << txn_data.total_price / txn_data.num << " " << txn_data.num;
  }
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
  hash_t user_ID_hash = SplitMix64Hash(user_name);
  if (online_users.find(user_ID_hash) == online_users.end()) {
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  b_plus_tree_value_index_t idx;
  bool success = false;
  idx = transaction_manager.FetchSingleUserOrderHistory(user_ID_hash, order, success);
  if (!success) {
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  TransactionData txn_data;
  transaction_manager.FetchTransactionData(idx, txn_data);
  if (txn_data.status == 2) {
    response_stream << "[" << command_id << "] -1";
    return response_stream.str();
  }
  if (txn_data.status == 0) {
    txn_data.status = 2;
    transaction_manager.UpdateTransactionData(idx, txn_data);
    // warning: the record in the queue is not deleted
    response_stream << "[" << command_id << "] 0";
    return response_stream.str();
  }
  txn_data.status = 2;
  transaction_manager.UpdateTransactionData(idx, txn_data);
  hash_t train_ID_hash = SplitMix64Hash(std::string_view(txn_data.trainID));
  hash_t from_station_hash = SplitMix64Hash(std::string_view(txn_data.from_station_name));
  hash_t to_station_hash = SplitMix64Hash(std::string_view(txn_data.to_station_name));
  SeatsData seats_data;
  seats_data_storage.Get({train_ID_hash, txn_data.running_date_offset}, seats_data);
  for (int i = txn_data.from_stop_id; i < txn_data.to_stop_id; i++) {
    seats_data.seat[i] += txn_data.num;
  }
  seats_data_storage.Put({train_ID_hash, txn_data.running_date_offset}, seats_data);
  std::vector<std::pair<b_plus_tree_value_index_t, uint32_t>> queue_idxs;
  transaction_manager.FetchQueue(train_ID_hash, txn_data.running_date_offset, queue_idxs);
  size_t len = queue_idxs.size();
  for (size_t i = 0; i < len; i++) {
    TransactionData cur_txn_data;
    transaction_manager.FetchTransactionData(queue_idxs[i].first, cur_txn_data);
    if (cur_txn_data.status != 0) {
      transaction_manager.RemoveOrderFromQueue(train_ID_hash, txn_data.running_date_offset, queue_idxs[i].second);
      continue;
    }
    int available_seats = seats_data.seat[cur_txn_data.from_stop_id];
    for (size_t j = cur_txn_data.from_stop_id + 1; j < cur_txn_data.to_stop_id; j++) {
      available_seats = std::min(available_seats, (int)seats_data.seat[j]);
    }
    if (available_seats >= cur_txn_data.num) {
      cur_txn_data.status = 1;
      transaction_manager.UpdateTransactionData(queue_idxs[i].first, cur_txn_data);
      for (int j = cur_txn_data.from_stop_id; j < cur_txn_data.to_stop_id; j++) {
        seats_data.seat[j] -= cur_txn_data.num;
      }
      seats_data_storage.Put({train_ID_hash, txn_data.running_date_offset}, seats_data);
      transaction_manager.RemoveOrderFromQueue(train_ID_hash, txn_data.running_date_offset, queue_idxs[i].second);
    }
  }
  response_stream << "[" << command_id << "] 0";
  return response_stream.str();
}