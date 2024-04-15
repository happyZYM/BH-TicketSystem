cmake_minimum_required(VERSION 3.10)
Project(Ticket-System)
include(CTest)
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

# 设置一个布尔类型的选项，用于控制是否启用日志功能
option(ENABLE_LOG "Enable logging" OFF)

# 如果 ENABLE_LOG 选项为 ON，则定义 ENABLE_LOG 宏
if (ENABLE_LOG)
    add_definitions(-DENABLE_LOG)
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
  URL_HASH SHA256=674e724c2702f0bfef1619161815257a407e1babce30d908327729fba6ce4124
  URL ${CMAKE_SOURCE_DIR}/deps/argparse-v3.0-mirror.zip
)
FetchContent_MakeAvailable(argparse)
FetchContent_Declare(
  spdlog
  URL_HASH SHA256=9f6763bb76fff7db371f5733626c83352edd7c57899501ab00248fafad9cc504
  URL ${CMAKE_SOURCE_DIR}/deps/spdlog-v1.13.0-mirror.zip
)
FetchContent_MakeAvailable(spdlog)
include_directories(${CMAKE_SOURCE_DIR}/bpt/include)
include_directories(${CMAKE_SOURCE_DIR}/stlite)
enable_testing()
add_subdirectory(test)
add_subdirectory(src)