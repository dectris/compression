cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(compression)

add_subdirectory(third_party EXCLUDE_FROM_ALL)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
  )

add_library(compression STATIC
  src/compression.c
  src/compression.h
  )
target_link_libraries(compression
  PRIVATE bitshuffle_core
  PRIVATE lz4
  )
