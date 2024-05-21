#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "engine.h"
#include "utils.h"

std::string TicketSystemEngine::BuyTicket(const std::string &command) { return "BuyTicket"; }

std::string TicketSystemEngine::QueryOrder(const std::string &command) { return "QueryOrder"; }

std::string TicketSystemEngine::RefundTicket(const std::string &command) { return "RefundTicket"; }

std::string TicketSystemEngine::QueryTransfer(const std::string &command) { return "QueryTransfer"; }

std::string TicketSystemEngine::QueryTicket(const std::string &command) { return "QueryTicket"; }