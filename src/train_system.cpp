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
        // TODO
        break;
      }
      case 'p': {
        std::string price_raw;
        command_stream >> price_raw;
        // TODO
        break;
      }
      case 'x': {
        command_stream >> startTime;
        break;
      }
      case 't': {
        std::string travelTime_raw;
        command_stream >> travelTime_raw;
        // TODO
        break;
      }
      case 'o': {
        std::string stopoverTime_raw;
        command_stream >> stopoverTime_raw;
        // TODO
        break;
      }
      case 'd': {
        std::string saleDate_raw;
        command_stream >> saleDate_raw;
        // TODO
        break;
      }
      case 'y': {
        command_stream >> type;
        break;
      }
    }
  }
  response_stream << '[' << command_id << "] AddTrain";
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
        // TODO
        break;
      }
    }
  }
  response_stream << '[' << command_id << "] QueryTrain";
  return response_stream.str();
}