#ifndef ENGINE_H
#define ENGINE_H
#include <map>
#include <string>
#ifdef ENABLE_ADVANCED_FEATURE
#include "dataguard/dataguard.h"
#include "dataguard/snapshot.h"
#endif
#include <vector>
#include "data.h"
#include "storage/disk_map.hpp"
#include "utils.h"
class TicketSystemEngine {
#ifdef ENABLE_ADVANCED_FEATURE
  SnapShotManager snapshot_manager;
#endif
  std::string data_directory;
  std::map<hash_t, uint8_t> online_users;
  DiskMap<hash_t, FullUserData> user_data;
  void PrepareExit();

 public:
  inline TicketSystemEngine(std::string data_directory)
      : data_directory(data_directory),
        user_data("user_data.idx", data_directory + "/user_data.idx", "user_data.val",
                  data_directory + "/user_data.val") {}
  std::string Execute(const std::string &command);

  // User system
  std::string AddUser(const std::string &command);
  std::string LoginUser(const std::string &command);
  std::string LogoutUser(const std::string &command);
  std::string QueryProfile(const std::string &command);
  std::string ModifyProfile(const std::string &command);

  // Train System
  std::string AddTrain(const std::string &command);
  std::string DeleteTrain(const std::string &command);
  std::string ReleaseTrain(const std::string &command);
  std::string QueryTrain(const std::string &command);

  // Transaction System
  std::string BuyTicket(const std::string &command);
  std::string QueryOrder(const std::string &command);
  std::string RefundTicket(const std::string &command);
  std::string QueryTransfer(const std::string &command);
  std::string QueryTicket(const std::string &command);

  // Other functions
  std::string Clean();
  std::string Exit(const std::string &command);
};
#endif