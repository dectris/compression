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

# No dictionary builder, no legacy formats and no x86-64 assembly.
file(GLOB ZSTD_SOURCES
  zstd/lib/common/*.c
  zstd/lib/compress/*.c
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

# Header-only TERSE/PROLIX v2 library. The headers of the git submodule are
# copied to the build directory and the patches in patches/trpx are applied,
# see the description in each patch file for why it is needed.
#
# The patches are applied with the same script as in setup.py instead of
# `patch`: it accepts LF and CRLF line endings (git core.autocrlf, the default
# on Windows), which `patch` implementations handle inconsistently.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(TRPX_PATCHED_DIR "${CMAKE_CURRENT_BINARY_DIR}/trpx")
set(APPLY_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/patches/apply_patches.py")
file(GLOB TRPX_PATCHES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/patches/trpx/*.patch"
  )
list(SORT TRPX_PATCHES)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${APPLY_PATCHES}" ${TRPX_PATCHES}
  )
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${APPLY_PATCHES}" --copy include
    "${CMAKE_CURRENT_SOURCE_DIR}/trpx" "${TRPX_PATCHED_DIR}" ${TRPX_PATCHES}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
  )
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Applying the patches in patches/trpx failed:\n${output}")
endif()

add_library(trpx INTERFACE)
target_include_directories(trpx SYSTEM INTERFACE
  "${TRPX_PATCHED_DIR}/include"
  )
