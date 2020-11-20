load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lz4",
    srcs = [
        "lib/lz4.c",
        "lib/lz4frame.c",
        "lib/lz4hc.c",
        "lib/xxhash.c",
        "lib/xxhash.h",
    ],
    hdrs = [
        "lib/lz4.h",
        "lib/lz4frame.h",
        "lib/lz4frame_static.h",
        "lib/lz4hc.h",
    ],
    textual_hdrs = [
        "lib/lz4.c",
    ],
)
