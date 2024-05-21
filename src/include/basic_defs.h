#ifndef BASIC_DEFS_H
#define BASIC_DEFS_H
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include <iostream>
#include <string>
extern const std::string main_version;
extern const std::string build_version;
extern std::shared_ptr<spdlog::logger> logger_ptr;
#ifndef ENABLE_ADVANCED_FEATURE
constexpr bool global_log_enabled = false;
#else
constexpr bool global_log_enabled = true;
#endif
extern const bool optimize_enabled;
#define LOG if constexpr (global_log_enabled) if (logger_ptr) logger_ptr
#endif