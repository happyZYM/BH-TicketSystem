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
#include "stop_register.hpp"
#include "storage/disk_map.hpp"
#include "transaction_mainenance.hpp"
#include "utils.h"
class TicketSystemEngine {
#ifdef ENABLE_ADVANCED_FEATURE
  SnapShotManager snapshot_manager;
#endif
  bool its_time_to_exit = false;
  std::string data_directory;
  std::map<hash_t, uint8_t> online_users;

  /**
   * @brief user data system
   * @details The database for users.
   */
  DiskMap<hash_t, FullUserData> user_data;

  /**
   * @brief train data system
   */
  DiskMap<hash_t, StationNameData> station_name_data_storage;
  DiskMap<hash_t, TicketPriceData> ticket_price_data_storage;
  DiskMap<hash_t, CoreTrainData> core_train_data_storage;
  typedef std::pair<hash_t, uint8_t> seats_index_t;
  DiskMap<seats_index_t, SeatsData> seats_data_storage;

  /**
   * @brief transaction data system
   * @details This part is responsible for storing:
   * - Stop information: using the station + HashedTrainID as the index, with a B+ tree point to the train's sales start
   * and end dates, minutes required from the starting station to the current station, and the current station's stop
   * time
   * - Order information: using an incrementing, non-repeating uint64_t as the index, with an OrderData attached
   * - Waiting queue: using the station + HashedTrainID as the index, with a B+ tree point to a LinkedQueue, storing IDs
   * pointing to order information
   * - User purchase history: using HashedUserID as the index, with a LinkedStack attached, with a simple skip list-like
   * optimization, storing IDs pointing to order information
   */
  StopRegister stop_register;
  TransactionManager transaction_manager;

  void PrepareExit();
  void CheckTransfer(hash_t train1_ID_hash, hash_t train2_ID_hash, const std::string &from_station,
                     const std::string &to_station, int date, bool &has_solution, std::string &res_train1_id,
                     std::string &res_train2_id, int &res_train1_leaving_time_stamp,
                     int &res_train1_arriving_time_stamp, int &res_train2_leaving_time_stamp,
                     int &res_train2_arriving_time_stamp, int &res_train1_price, int &res_train1_seat,
                     int &res_train2_price, int &res_train2_seat, std::string &res_transfer_station_name, bool sort_by_time);

 public:
  const bool *its_time_to_exit_ptr = &its_time_to_exit;
  inline TicketSystemEngine(std::string data_directory)
      : data_directory(data_directory),
        user_data("user_data.idx", data_directory + "/user_data.idx", "user_data.val",
                  data_directory + "/user_data.val"),
        station_name_data_storage("station_name.idx", data_directory + "/station_name.idx", "station_name.val",
                                  data_directory + "/station_name.val"),
        ticket_price_data_storage("ticket_price.idx", data_directory + "/ticket_price.idx", "ticket_price.val",
                                  data_directory + "/ticket_price.val"),
        core_train_data_storage("core_train.idx", data_directory + "/core_train.idx", "core_train.val",
                                data_directory + "/core_train.val"),
        seats_data_storage("seats.idx", data_directory + "/seats.idx", "seats.val", data_directory + "/seats.val"),
        stop_register("stop_register.idx", data_directory + "/stop_register.idx"),
        transaction_manager("txn.data", data_directory + "/txn.data", "queue.idx", data_directory + "/queue.idx",
                            "order.idx", data_directory + "/order.idx") {}
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