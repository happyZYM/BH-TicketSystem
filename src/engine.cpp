#include "engine.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "utils.h"
const hash_t add_user_hash = 1294763820278197867ull;         // SplitMix64Hash(std::string_view("add_user"));
const hash_t login_hash = 2711532776857333909ull;            // SplitMix64Hash(std::string_view("login"));
const hash_t logout_hash = 654844766916416630ull;            // SplitMix64Hash(std::string_view("logout"));
const hash_t query_profile_hash = 2497292575416072607ull;    // SplitMix64Hash(std::string_view("query_profile"));
const hash_t modify_profile_hash = 11664297803074461165ull;  // SplitMix64Hash(std::string_view("modify_profile"));
const hash_t add_train_hash = 4928024213250587878ull;        // SplitMix64Hash(std::string_view("add_train"));
const hash_t delete_train_hash = 13574898832952622188ull;    // SplitMix64Hash(std::string_view("delete_train"));
const hash_t release_train_hash = 3024733565973464225ull;    // SplitMix64Hash(std::string_view("release_train"));
const hash_t query_train_hash = 8200860784912964124ull;      // SplitMix64Hash(std::string_view("query_train"));
const hash_t query_ticket_hash = 11565392281772224130ull;    // SplitMix64Hash(std::string_view("query_ticket"));
const hash_t query_transfer_hash = 17604853834584868005ull;  // SplitMix64Hash(std::string_view("query_transfer"));
const hash_t buy_ticket_hash = 16906761384093040506ull;      // SplitMix64Hash(std::string_view("buy_ticket"));
const hash_t query_order_hash = 8097745745927766409ull;      // SplitMix64Hash(std::string_view("query_order"));
const hash_t refund_ticket_hash = 14136625827132030759ull;   // SplitMix64Hash(std::string_view("refund_ticket"));
const hash_t clean_hash = 13563833734274431010ull;           // SplitMix64Hash(std::string_view("clean"));
const hash_t exit_hash = 5825494509148032335ull;             // SplitMix64Hash(std::string_view("exit"));
std::string TicketSystemEngine::Execute(const std::string &command) {
  // LOG->debug("add_user_hash: {}", add_user_hash);
  // LOG->debug("login_hash: {}", login_hash);
  // LOG->debug("logout_hash: {}", logout_hash);
  // LOG->debug("query_profile_hash: {}", query_profile_hash);
  // LOG->debug("modify_profile_hash: {}", modify_profile_hash);
  // LOG->debug("add_train_hash: {}", add_train_hash);
  // LOG->debug("delete_train_hash: {}", delete_train_hash);
  // LOG->debug("release_train_hash: {}", release_train_hash);
  // LOG->debug("query_train_hash: {}", query_train_hash);
  // LOG->debug("query_ticket_hash: {}", query_ticket_hash);
  // LOG->debug("query_transfer_hash: {}", query_transfer_hash);
  // LOG->debug("buy_ticket_hash: {}", buy_ticket_hash);
  // LOG->debug("query_order_hash: {}", query_order_hash);
  // LOG->debug("refund_ticket_hash: {}", refund_ticket_hash);
  // LOG->debug("clean_hash: {}", clean_hash);
  // LOG->debug("exit_hash: {}", exit_hash);
  char command_name[20];
  sscanf(command.c_str(), "%*s %s", command_name);
  hash_t command_name_hash = SplitMix64Hash(std::string_view(command_name));
  switch (command_name_hash) {
    case add_user_hash:
      LOG->debug("match add_user");
      return std::move(AddUser(command));
    case login_hash:
      LOG->debug("match login");
      return std::move(LoginUser(command));
    case logout_hash:
      LOG->debug("match logout");
      return std::move(LogoutUser(command));
    case query_profile_hash:
      LOG->debug("match query_profile");
      return std::move(QueryProfile(command));
    case modify_profile_hash:
      LOG->debug("match modify_profile");
      return std::move(ModifyProfile(command));
    case add_train_hash:
      LOG->debug("match add_train");
      return std::move(AddTrain(command));
    case delete_train_hash:
      LOG->debug("match delete_train");
      return std::move(DeleteTrain(command));
    case release_train_hash:
      LOG->debug("match release_train");
      return std::move(ReleaseTrain(command));
    case query_train_hash:
      LOG->debug("match query_train");
      return std::move(QueryTrain(command));
    case query_ticket_hash:
      LOG->debug("match query_ticket");
      return std::move(QueryTicket(command));
    case query_transfer_hash:
      LOG->debug("match query_transfer");
      return std::move(QueryTransfer(command));
    case buy_ticket_hash:
      LOG->debug("match buy_ticket");
      return std::move(BuyTicket(command));
    case query_order_hash:
      LOG->debug("match query_order");
      return std::move(QueryOrder(command));
    case refund_ticket_hash:
      LOG->debug("match refund_ticket_hash");
      return std::move(RefundTicket(command));
    case clean_hash:
      LOG->debug("match clean");
      return std::move(Clean());
    case exit_hash:
      LOG->debug("match exit");
      PrepareExit();
      return std::move(Exit(command));
  }
  throw std::invalid_argument("Invalid command.");
}

std::string TicketSystemEngine::Clean() { throw std::runtime_error("Command clean is not implemented"); }

std::string TicketSystemEngine::Exit(const std::string &command) {
  command_id_t command_id;
  sscanf(command.c_str(), "[%llu]", &command_id);
  LOG->debug("command id: {}", command_id);
  std::stringstream response_stream;
  response_stream << '[' << command_id << "] bye";
  its_time_to_exit = true;
  return response_stream.str();
}

void TicketSystemEngine::PrepareExit() { LOG->info("Preparing exit"); }