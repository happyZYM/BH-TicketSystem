#include <sockpp/tcp_acceptor.h>
#include "basic_defs.h"
const std::string main_version = "0.0.1";
const std::string build_version = GIT_COMMIT_HASH;
std::shared_ptr<spdlog::logger> logger_ptr;
#ifdef __OPTIMIZE__
const bool optimize_enabled = __OPTIMIZE__;
#else
const bool optimize_enabled = false;
#endif
#ifndef ENABLE_LOG
const bool global_log_enabled = false;
#else
const bool global_log_enabled = true;
#endif
int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("core-cli", main_version + "-" + build_version);
  argparse::ArgumentParser fsck_command("fsck");
  fsck_command.add_description("Check and fix data");
  program.add_subparser(fsck_command);
  argparse::ArgumentParser server_command("server");
  server_command.add_description("Start a socket server");
  server_command.add_argument("-p", "--port").help("Port to listen").default_value(8085).nargs(1, 1).scan<'i', int>();
  server_command.add_argument("-a", "--address")
      .help("Address to bind")
      .default_value(std::string("127.0.0.1"))
      .nargs(1, 1);
  program.add_subparser(server_command);
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
  LOG->info("Starting backend");
  LOG->info("Compile optimization enabled: {}", optimize_enabled);
  LOG->info("Data directory: {}", data_directory);
  bool is_server = program.is_subcommand_used("server");
  LOG->info("Server mode: {}", is_server);
  if (is_server) {
    auto port = server_command.get<int>("--port");
    auto address = server_command.get<std::string>("--address");
    LOG->info("Server port: {}", port);
    LOG->info("Server address: {}", address);
    LOG->info("Starting server");
    sockpp::tcp_acceptor acceptor(sockpp::inet_address(address, port));
    if (!acceptor) {
      LOG->error("Error creating acceptor: {}", acceptor.last_error_str());
      return 1;
    } else
      LOG->info("successfully bind to address {} port {}", address, port);
  }
  return 0;
}