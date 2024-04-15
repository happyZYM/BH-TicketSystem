#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include <iostream>
#include <string>
const std::string main_version = "0.0.1";
const std::string build_version = GIT_COMMIT_HASH;
std::shared_ptr<spdlog::logger> logger_ptr;
int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("backend", main_version + "-" + build_version);
  argparse::ArgumentParser fsck_command("fsck");
  fsck_command.add_description("Check and fix data");
  program.add_subparser(fsck_command);
  program.add_argument("-d", "--directory").help("Directory to serve").default_value(std::string(".")).nargs(1, 1);
  auto &group = program.add_mutually_exclusive_group();
  group.add_argument("-c", "--consolelog").help("Enable console log").default_value(false).implicit_value(true);
  group.add_argument("-l", "--logfile").help("Enable log file").nargs(1, 1);
  program.add_argument("--level")
      .help("Log level")
      .default_value(std::string("info"))
      .nargs(1, 1)
      .choices("debug", "info", "warn", "error");
  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }
  auto data_directory = program.get<std::string>("--directory");
  bool log_enabled = program.get<bool>("--consolelog");
  std::string log_file_name;
  if (auto it = program.present("--logfile")) {
    log_enabled = true;
    log_file_name = program.get<std::string>("--logfile");
  }
  std::string log_level = program.get<std::string>("--level");
  if (log_level == "debug")
    spdlog::set_level(spdlog::level::debug);
  else if (log_level == "info")
    spdlog::set_level(spdlog::level::info);
  else if (log_level == "warn")
    spdlog::set_level(spdlog::level::warn);
  else if (log_level == "error")
    spdlog::set_level(spdlog::level::err);
  else {
    std::cerr << "Invalid log level" << std::endl;
    return 1;
  }
  if (log_enabled) {
    if (log_file_name == "")
      logger_ptr = spdlog::stderr_color_mt("stderr_logger");
    else
      logger_ptr = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", log_file_name);
  }
  if (logger_ptr) logger_ptr->info("Starting backend");
  if (logger_ptr) logger_ptr->info("Data directory: {}", data_directory);
  return 0;
}