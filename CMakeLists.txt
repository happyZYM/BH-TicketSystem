cmake_minimum_required(VERSION 3.10)
Project(Ticket-System)
include(CTest)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address -fsanitize=undefined -fsanitize=leak")
option(TEST_BPT_ONLY "Only build BPT tests" ON)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL_HASH SHA256=1f357c27ca988c3f7c6b4bf68a9395005ac6761f034046e9dde0896e3aba00e4
  URL ${CMAKE_SOURCE_DIR}/deps/googletest-v1.14.0-mirror.zip
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
FetchContent_Declare(
  argparse
  URL_HASH SHA256=674e724c2702f0bfef1619161815257a407e1babce30d908327729fba6ce4124
  URL ${CMAKE_SOURCE_DIR}/deps/argparse-v3.0-mirror.zip
)
FetchContent_MakeAvailable(argparse)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/bpt/include)
include_directories(${CMAKE_SOURCE_DIR}/stlite)
enable_testing()
add_subdirectory(test)