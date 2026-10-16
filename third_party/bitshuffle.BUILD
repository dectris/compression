load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bitshuffle",
    srcs = [
        "src/bitshuffle.c",
    ],
    hdrs = [
        "src/bitshuffle.h",
    ],
    strip_include_prefix = "src",
)
