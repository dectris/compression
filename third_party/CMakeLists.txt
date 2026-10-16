add_library(bitshuffle STATIC
  bitshuffle/src/bitshuffle.c
  bitshuffle/src/bitshuffle.h
  )
target_include_directories(bitshuffle PUBLIC bitshuffle/src)

add_library(lz4 STATIC
  lz4/lib/lz4.c
  lz4/lib/lz4.h
  )
target_include_directories(lz4 PUBLIC lz4/lib)

# Decompression only, no legacy formats and no x86-64 assembly.
file(GLOB ZSTD_SOURCES
  zstd/lib/common/*.c
  zstd/lib/decompress/*.c
  )
add_library(zstd STATIC
  ${ZSTD_SOURCES}
  zstd/lib/zstd.h
  )
target_include_directories(zstd PUBLIC zstd/lib)
target_compile_definitions(zstd PRIVATE
  ZSTD_DISABLE_ASM=1
  ZSTD_LEGACY_SUPPORT=0
  )

# Header-only TERSE/PROLIX v2 library.
add_library(trpx INTERFACE)
target_include_directories(trpx SYSTEM INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/trpx/include"
  )
