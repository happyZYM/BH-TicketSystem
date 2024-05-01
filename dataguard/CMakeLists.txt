add_library(dataguard STATIC src/snapshot.cpp src/txn_logger.cpp)
target_link_libraries(dataguard libzstd_static storage)