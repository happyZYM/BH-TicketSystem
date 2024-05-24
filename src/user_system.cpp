#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "engine.h"
#include "utils.h"

std::string TicketSystemEngine::AddUser(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  LOG->debug("command: {}", command);
  std::stringstream command_stream(command), response_stream;
  std::string token, cur_username, username, password, name, mailAddr;
  uint8_t privilege;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'c': {
        command_stream >> cur_username;
        break;
      }
      case 'u': {
        command_stream >> username;
        break;
      }
      case 'p': {
        command_stream >> password;
        break;
      }
      case 'n': {
        command_stream >> name;
        break;
      }
      case 'm': {
        command_stream >> mailAddr;
        break;
      }
      case 'g': {
        int tmp;
        command_stream >> tmp;
        privilege = tmp;
        break;
      }
      default:
        throw std::invalid_argument("arg parse fatal error in add_user");
    }
  }
  if (user_data.size() == 0) {
    // special case, no need to check current user's privilege
    FullUserData dat;
    dat.privilege = 10;
    strcpy(dat.username, username.c_str());
    dat.password_hash = SplitMix64Hash(password);
    strcpy(dat.name, name.c_str());
    strcpy(dat.mailAddr, mailAddr.c_str());
    user_data.Put(SplitMix64Hash(username), dat);
    transaction_manager.PrepareUserInfo(SplitMix64Hash(username));
    LOG->debug("stored user_name hash: {}", SplitMix64Hash(username));
    LOG->debug("stored user_name: {}", dat.username);
    LOG->debug("stored password hash: {}", dat.password_hash);
    LOG->debug("stored name: {}", dat.name);
    LOG->debug("stored mailAddr: {}", dat.mailAddr);
    LOG->debug("stored privilege: {}", dat.privilege);
    response_stream << '[' << command_id << "] 0";
    return response_stream.str();
  }
  hash_t current_user_username_hash = SplitMix64Hash(cur_username);
  if (online_users.find(current_user_username_hash) == online_users.end()) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  if (privilege >= online_users[current_user_username_hash]) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  hash_t new_user_username_hash = SplitMix64Hash(username);
  if (user_data.HasKey(new_user_username_hash)) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  FullUserData dat;
  dat.privilege = privilege;
  strcpy(dat.username, username.c_str());
  dat.password_hash = SplitMix64Hash(password);
  strcpy(dat.name, name.c_str());
  strcpy(dat.mailAddr, mailAddr.c_str());
  user_data.Put(new_user_username_hash, dat);
  LOG->debug("stored user_name hash: {}", new_user_username_hash);
  LOG->debug("stored user_name: {}", dat.username);
  LOG->debug("stored password hash: {}", dat.password_hash);
  LOG->debug("stored name: {}", dat.name);
  LOG->debug("stored mailAddr: {}", dat.mailAddr);
  LOG->debug("stored privilege: {}", dat.privilege);
  response_stream << '[' << command_id << "] 0";
  return response_stream.str();
}

std::string TicketSystemEngine::LoginUser(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token, user_name, password;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'u': {
        command_stream >> user_name;
        break;
      }
      case 'p': {
        command_stream >> password;
        break;
      }
      default:
        throw std::invalid_argument("arg parse fatal error in login");
    }
  }
  hash_t user_name_hash = SplitMix64Hash(user_name);
  hash_t password_hash = SplitMix64Hash(password);
  if (online_users.find(user_name_hash) != online_users.end()) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  FullUserData dat;
  try {
    user_data.Get(user_name_hash, dat);
    if (dat.password_hash != password_hash) {
      response_stream << '[' << command_id << "] -1";
      return response_stream.str();
    }
  } catch (std::runtime_error &e) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  online_users[user_name_hash] = dat.privilege;
  response_stream << '[' << command_id << "] 0";
  return response_stream.str();
}

std::string TicketSystemEngine::LogoutUser(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token, user_name;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'u': {
        command_stream >> user_name;
        break;
      }
      default:
        throw std::invalid_argument("arg parse fatal error in logout");
    }
  }
  hash_t user_name_hash = SplitMix64Hash(user_name);
  if (online_users.find(user_name_hash) == online_users.end()) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  online_users.erase(user_name_hash);
  response_stream << '[' << command_id << "] 0";
  return response_stream.str();
}

std::string TicketSystemEngine::QueryProfile(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token, current_user_name, user_name;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'c': {
        command_stream >> current_user_name;
        break;
      }
      case 'u': {
        command_stream >> user_name;
        break;
      }
      default:
        throw std::invalid_argument("arg parse fatal error in query_profile");
    }
  }
  hash_t current_user_name_hash = SplitMix64Hash(current_user_name);
  hash_t user_name_hash = SplitMix64Hash(user_name);
  if (online_users.find(current_user_name_hash) == online_users.end()) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  FullUserData dat;
  LOG->debug("user_name_hash: {}", user_name_hash);
  LOG->debug("mailAddr: {}", dat.mailAddr);
  if (current_user_name_hash != user_name_hash) {
    try {
      user_data.Get(user_name_hash, dat);
      if (online_users[current_user_name_hash] <= dat.privilege) {
        response_stream << '[' << command_id << "] -1";
        return response_stream.str();
      }
    } catch (std::runtime_error &e) {
      response_stream << '[' << command_id << "] -1";
      return response_stream.str();
    }
  } else {
    user_data.Get(user_name_hash, dat);
  }
  LOG->debug("mailAddr: {}", dat.mailAddr);
  response_stream << '[' << command_id << "] " << dat.username << ' ' << dat.name << ' ' << dat.mailAddr << ' '
                  << static_cast<int>(dat.privilege);
  return response_stream.str();
}

std::string TicketSystemEngine::ModifyProfile(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream command_stream(command), response_stream;
  std::string token, current_user_name, user_name, password, name, mailAddr;
  uint8_t privilege = 11;
  command_stream >> token >> token;
  while (command_stream >> token) {
    switch (token[1]) {
      case 'c': {
        command_stream >> current_user_name;
        break;
      }
      case 'u': {
        command_stream >> user_name;
        break;
      }
      case 'p': {
        command_stream >> password;
        break;
      }
      case 'n': {
        command_stream >> name;
        break;
      }
      case 'm': {
        command_stream >> mailAddr;
        break;
      }
      case 'g': {
        int tmp;
        command_stream >> tmp;
        privilege = tmp;
        break;
      }
      default:
        throw std::invalid_argument("arg parse fatal error in add_user");
    }
  }
  hash_t current_user_name_hash = SplitMix64Hash(current_user_name);
  hash_t user_name_hash = SplitMix64Hash(user_name);
  if (online_users.find(current_user_name_hash) == online_users.end()) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  FullUserData dat;
  if (current_user_name_hash != user_name_hash) {
    try {
      user_data.Get(user_name_hash, dat);
      if (online_users[current_user_name_hash] <= dat.privilege) {
        response_stream << '[' << command_id << "] -1";
        return response_stream.str();
      }
    } catch (std::runtime_error &e) {
      response_stream << '[' << command_id << "] -1";
      return response_stream.str();
    }
  } else {
    user_data.Get(user_name_hash, dat);
  }
  if (privilege != 11 && privilege >= online_users[current_user_name_hash]) {
    response_stream << '[' << command_id << "] -1";
    return response_stream.str();
  }
  if (privilege != 11) {
    dat.privilege = privilege;
  }
  if (password != "") {
    dat.password_hash = SplitMix64Hash(password);
  }
  if (name != "") {
    strcpy(dat.name, name.c_str());
  }
  if (mailAddr != "") {
    strcpy(dat.mailAddr, mailAddr.c_str());
  }
  user_data.Put(user_name_hash, dat);
  response_stream << '[' << command_id << "] " << dat.username << ' ' << dat.name << ' ' << dat.mailAddr << ' '
                  << static_cast<int>(dat.privilege);
  return response_stream.str();
}