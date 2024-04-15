#include <argparse/argparse.hpp>
#include <string>
const std::string main_version = "0.0.1";
const std::string build_version = GIT_COMMIT_HASH;
int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("backend", main_version + "-" + build_version);
  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }
  return 0;
}