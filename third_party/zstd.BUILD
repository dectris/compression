load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# No legacy formats and no x86-64 assembly (portable C fallback).
ZSTD_LOCAL_DEFINES = [
    "ZSTD_DISABLE_ASM=1",
    "ZSTD_LEGACY_SUPPORT=0",
]

# Decompression only.
cc_library(
    name = "zstd",
    srcs = glob([
        "lib/common/*.c",
        "lib/common/*.h",
        "lib/decompress/*.c",
        "lib/decompress/*.h",
    ]),
    hdrs = [
        "lib/zstd.h",
        "lib/zstd_errors.h",
    ],
    local_defines = ZSTD_LOCAL_DEFINES,
)

# Compression (for tests).
cc_library(
    name = "zstd_compress",
    srcs = glob([
        "lib/compress/*.c",
        "lib/compress/*.h",
    ]),
    local_defines = ZSTD_LOCAL_DEFINES,
    deps = [":zstd"],
)
