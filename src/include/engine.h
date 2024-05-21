#ifndef ENGINE_H
#define ENGINE_H
#include <map>
#include <string>
#ifdef ENABLE_ADVANCED_FEATURE
#include "dataguard/dataguard.h"
#include "dataguard/snapshot.h"
#endif
#include <vector>
#include "utils.h"
class TicketSystemEngine {
#ifdef ENABLE_ADVANCED_FEATURE
  SnapShotManager snapshot_manager;
#endif
  std::string data_directory;
  std::map<hash_t, bool> online_users;
  void PrepareExit();

 public:
  inline TicketSystemEngine(std::string data_directory) : data_directory(data_directory) {}
  std::string Execute(const std::string &command);

  // 用户相关函数
  std::string AddUser(const std::string &command);
  std::string LoginUser(const std::string &command);
  std::string LogoutUser(const std::string &command);
  std::string QueryProfile(const std::string &command);
  std::string ModifyProfile(const std::string &command);

  // 车次相关函数
  std::string AddTrain(const std::string &command);
  std::string DeleteTrain(const std::string &command);
  std::string ReleaseTrain(const std::string &command);
  std::string QueryTrain(const std::string &command);

  // 订单相关函数
  std::string BuyTicket(const std::string &command);
  std::string QueryOrder(const std::string &command);
  std::string RefundTicket(const std::string &command);

  // 其他函数
  std::string QueryTransfer(const std::string &command);
  std::string QueryTicket(const std::string &command);
  std::string Clean();
  std::string Exit();
};
#endif