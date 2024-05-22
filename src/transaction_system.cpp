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
        // TODO: parse date
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
  response_stream << "[" << command_id << "] QueryTicket";
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
        // TODO: parse date
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
        // TODO: parse date
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