#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "basic_defs.h"
#include "data.h"
#include "engine.h"
#include "utils.h"

std::string TicketSystemEngine::AddTrain(const std::string &command) { return "AddTrain"; }

std::string TicketSystemEngine::DeleteTrain(const std::string &command) { return "DeleteTrain"; }

std::string TicketSystemEngine::ReleaseTrain(const std::string &command) { return "ReleaseTrain"; }

std::string TicketSystemEngine::QueryTrain(const std::string &command) { return "QueryTrain"; }