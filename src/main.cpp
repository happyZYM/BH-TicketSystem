#include <cassert>
#include <exception>
#include "basic_defs.h"
#ifdef ENABLE_ADVANCED_FEATURE
#include <sockpp/tcp_acceptor.h>
#include <thread>
#include "dataguard/dataguard.h"
#endif
#include "engine.h"
#include "storage/bpt.hpp"
const std::string main_version = "0.0.1";
const std::string build_version = GIT_COMMIT_HASH;
std::shared_ptr<spdlog::logger> logger_ptr;
#ifdef __OPTIMIZE__
const bool optimize_enabled = __OPTIMIZE__;
#else
const bool optimize_enabled = false;
#endif
// #ifndef ENABLE_ADVANCED_FEATURE
// const bool global_log_enabled = false;
// #else
// const bool global_log_enabled = true;
// #endif

#ifdef ENABLE_ADVANCED_FEATURE

// 处理每个连接的函数
void handle_client(sockpp::tcp_socket client, TicketSystemEngine &engine) {
  try {
    std::string line;
    std::string buffer;

    // 读取客户端发送的数据
    while (true) {
      char data[1024];
      ssize_t n = client.read(data, sizeof(data));
      if (n <= 0) {
        break;  // 连接关闭或出现错误
      }
      buffer.append(data, n);

      // 检查是否收到完整的一行
      size_t pos;
      while ((pos = buffer.find('\n')) != std::string::npos) {
        line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        line = "[0] " + line;

        // 调用 TicketSystemEngine 的 Execute 方法处理命令
        std::string response = engine.Execute(line);

        // 发送响应给客户端
        client.write(response);

        // 检查是否需要退出
        if (*engine.its_time_to_exit_ptr) {
          exit(0);
          return;
        }
      }
    }
  } catch (const std::exception &e) {
    LOG->error("Exception handling client: {}", e.what());
  }
}

#endif  // ENABLE_ADVANCED_FEATURE

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("zts-core", main_version + "-" + build_version);
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
  argparse::ArgumentParser snapshot_command("snapshot");
  snapshot_command.add_description("Manage snapshots");
  program.add_subparser(snapshot_command);
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
    try {
      if (log_file_name == "")
        logger_ptr = spdlog::stderr_color_mt("stderr_logger");
      else
        logger_ptr = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", log_file_name);
    } catch (const spdlog::spdlog_ex &ex) {
      std::cerr << "Log initialization failed: " << ex.what() << std::endl;
      return 1;
    }
  }
  LOG->info("Starting backend");
  LOG->info("Compile optimization enabled: {}", optimize_enabled);
  LOG->info("Data directory: {}", data_directory);
  bool is_server = program.is_subcommand_used("server");
  LOG->info("Server mode: {}", is_server);
  try {
#ifdef ENABLE_ADVANCED_FEATURE
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
      // throw std::runtime_error("Server mode not implemented");
      TicketSystemEngine engine(data_directory);
      while (true) {
        // 接受新的客户端连接
        sockpp::tcp_socket client = acceptor.accept();
        if (!client) {
          LOG->error("Error accepting incoming connection: {}", acceptor.last_error_str());
        } else {
          LOG->info("New client connected from {}", client.peer_address().to_string());

          // 创建一个线程来处理客户端连接
          std::thread(handle_client, std::move(client), std::ref(engine)).detach();
        }
      }
    } else {
#endif
      std::ios::sync_with_stdio(false);
      std::cin.tie(nullptr);
      std::cout.tie(nullptr);
      TicketSystemEngine engine(data_directory);
      std::string cmd;
      while (std::getline(std::cin, cmd)) {
        std::cout << engine.Execute(cmd) << '\n';
#ifdef DISABLE_COUT_CACHE
        std::cout.flush();
#endif
        if (*engine.its_time_to_exit_ptr) break;
      }
#ifdef ENABLE_ADVANCED_FEATURE
    }
#endif
  } catch (std::exception &e) {
    LOG->error("Exception: {}", e.what());
    return 1;
  } catch (...) {
    LOG->error("Unknown exception");
    return 2;
  }
  return 0;
}