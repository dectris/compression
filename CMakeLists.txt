cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(compression C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_subdirectory(third_party EXCLUDE_FROM_ALL)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

add_library(compression STATIC
  src/compression.c
  src/compression.h
  src/trpx.cc
  src/trpx.h
  )
target_link_libraries(compression
  PRIVATE bitshuffle
  PRIVATE lz4
  PRIVATE zstd
  PRIVATE trpx
  PRIVATE Threads::Threads
  )
