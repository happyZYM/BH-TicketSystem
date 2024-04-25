add_library(snapshot STATIC src/snapshot.cpp)
target_link_libraries(snapshot libzstd_static)