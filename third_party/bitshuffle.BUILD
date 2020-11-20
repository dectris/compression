load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bitshuffle_core",
    srcs = [
        "src/bitshuffle_core.c",
        "src/iochain.c",
    ],
    hdrs = [
        "src/bitshuffle_core.h",
        "src/bitshuffle_internals.h",
        "src/iochain.h",
    ],
)

cc_library(
    name = "bitshuffle",
    srcs = [
        "src/bitshuffle.c",
    ],
    hdrs = [
        "src/bitshuffle.h",
    ],
    copts = [
        "-Iexternal/lz4/lib",
    ],
    deps = [
        ":bitshuffle_core",
        "@lz4",
    ],
)

cc_library(
    name = "hdf5_filter",
    srcs = [
        "src/bshuf_h5filter.c",
        "src/bshuf_h5plugin.c",
    ],
    hdrs = [
        "src/bshuf_h5filter.h",
    ],
    deps = [
        ":bitshuffle",
        "@hdf5",
    ],
)
