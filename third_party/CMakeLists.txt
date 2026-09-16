add_library(bitshuffle STATIC
  bitshuffle/src/bitshuffle.c
  bitshuffle/src/bitshuffle.h
  )

add_library(lz4 STATIC
  lz4/lib/lz4.c
  lz4/lib/lz4.h
  )

# Decompression only, no legacy formats and no x86-64 assembly.
file(GLOB ZSTD_SOURCES
  zstd/lib/common/*.c
  zstd/lib/decompress/*.c
  )
add_library(zstd STATIC
  ${ZSTD_SOURCES}
  zstd/lib/zstd.h
  )
target_compile_definitions(zstd PRIVATE
  ZSTD_DISABLE_ASM=1
  ZSTD_LEGACY_SUPPORT=0
  )

# Header-only TERSE/PROLIX v2 library. The headers of the git submodule are
# copied to the build directory and the patches in patches/trpx are applied,
# see the description in each patch file for why it is needed.
#
# Git may check out the headers with CRLF line endings (core.autocrlf, the
# default on Windows), which `patch` rejects. The headers and the patches are
# therefore copied with LF line endings before the patches are applied.
find_package(Patch REQUIRED)
set(TRPX_PATCHED_DIR "${CMAKE_CURRENT_BINARY_DIR}/trpx")
set(TRPX_PATCHES_DIR "${CMAKE_CURRENT_BINARY_DIR}/trpx-patches")
file(GLOB TRPX_PATCHES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/patches/trpx/*.patch"
  )
list(SORT TRPX_PATCHES)
file(GLOB TRPX_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/trpx"
  "${CMAKE_CURRENT_SOURCE_DIR}/trpx/include/*"
  )

function(copy_with_lf_line_endings src dst)
  file(READ "${src}" content)
  string(REPLACE "\r\n" "\n" content "${content}")
  file(WRITE "${dst}" "${content}")
endfunction()

file(REMOVE_RECURSE "${TRPX_PATCHED_DIR}" "${TRPX_PATCHES_DIR}")
foreach(header IN LISTS TRPX_HEADERS)
  copy_with_lf_line_endings("${CMAKE_CURRENT_SOURCE_DIR}/trpx/${header}"
    "${TRPX_PATCHED_DIR}/${header}")
endforeach()
foreach(patch IN LISTS TRPX_PATCHES)
  get_filename_component(name "${patch}" NAME)
  copy_with_lf_line_endings("${patch}" "${TRPX_PATCHES_DIR}/${name}")
  execute_process(
    COMMAND "${Patch_EXECUTABLE}" -p1 --binary --input "${TRPX_PATCHES_DIR}/${name}"
    WORKING_DIRECTORY "${TRPX_PATCHED_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Applying ${patch} failed:\n${output}")
  endif()
endforeach()

add_library(trpx INTERFACE)
target_include_directories(trpx SYSTEM INTERFACE
  "${TRPX_PATCHED_DIR}/include"
  )
