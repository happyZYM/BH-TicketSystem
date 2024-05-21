cmake_minimum_required(VERSION 3.10)
Project(Ticket-System)

set(CMAKE_CXX_STANDARD 20)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address -fsanitize=undefined -fsanitize=leak")

# 执行git命令检查是否有未存档的更改
execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_STATUS_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 判断输出是否为空，如果有未存档的更改则不做处理
string(LENGTH "${GIT_STATUS_OUTPUT}" STATUS_LENGTH)
if(STATUS_LENGTH EQUAL 0)
  # 执行git命令获取commit信息
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  # 定义宏传递commit信息给C++程序
  add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
else()
  add_definitions(-DGIT_COMMIT_HASH="[developing]")
endif()
add_definitions(-DZSTD_STATIC_LINKING_ONLY)

# 设置一个布尔类型的选项，用于控制是否启用高级功能，如日志（业务日志、程序日志、容错校验日志）、并发、快照等
option(ENABLE_ADVANCED_FEATURE "Enable advanced features" OFF)
option(OJ_TEST_BPT "Enable OJ test for B+ Tree" OFF)
option(OJ_TEST_BACKEND "Enable OJ test for backend" ON)

# 如果 ENABLE_ADVANCED_FEATURE 选项为 ON，则定义 ENABLE_ADVANCED_FEATURE 宏
if (ENABLE_ADVANCED_FEATURE)
    add_definitions(-DENABLE_ADVANCED_FEATURE)
endif()

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
  URL_HASH SHA256=cd07c1208c01bef28c5173f4bad0b2df73dd7316d2f56fc80344952c400fa711
  URL ${CMAKE_SOURCE_DIR}/deps/argparse-9550b0a-mirror.zip
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
  spdlog
  URL_HASH SHA256=9f6763bb76fff7db371f5733626c83352edd7c57899501ab00248fafad9cc504
  URL ${CMAKE_SOURCE_DIR}/deps/spdlog-v1.13.0-mirror.zip
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  sockpp
  URL_HASH SHA256=d144b3dcc7e9f031a9205395deb334b0b9681e1e7db61c066cca22ec2650a77b
  URL ${CMAKE_SOURCE_DIR}/deps/sockpp-v1.0.0-mirror.zip
)
FetchContent_MakeAvailable(sockpp)

if(ENABLE_ADVANCED_FEATURE)
  FetchContent_Declare(
    zstd
    URL_HASH SHA256=3b1c3b46e416d36931efd34663122d7f51b550c87f74de2d38249516fe7d8be5
    URL ${CMAKE_SOURCE_DIR}/deps/zstd-v1.5.6-mirror.zip
    SOURCE_SUBDIR build/cmake
  )
  set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build shared libraries")
  set(ZSTD_BUILD_DEPRECATED OFF CACHE BOOL "Build deprecated module")
  FetchContent_MakeAvailable(zstd)
endif()

include_directories(${CMAKE_SOURCE_DIR}/storage/include)
if(ENABLE_ADVANCED_FEATURE)
  include_directories(${CMAKE_SOURCE_DIR}/dataguard/include)
endif()
include_directories(${CMAKE_SOURCE_DIR}/stlite)

include(CTest)
enable_testing()
include(test/ctest_config)

add_subdirectory(storage)
if(ENABLE_ADVANCED_FEATURE)
  add_subdirectory(dataguard)
endif()
add_subdirectory(test)
add_subdirectory(src)