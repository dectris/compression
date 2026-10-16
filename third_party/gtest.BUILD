load("@rules_cc//cc:defs.bzl", "cc_library")

# GoogleTest requires C++14. The Bazel toolchains for GCC and Clang default to
# -std=c++0x, MSVC defaults to C++14.
COPTS = select({
    "@platforms//os:windows": [],
    "//conditions:default": ["-std=c++14"],
})

cc_library(
    name = "gtest",
    srcs = [
        "googlemock/src/gmock-all.cc",
        "googletest/src/gtest-all.cc",
    ],
    copts = COPTS,
    hdrs = glob([
        "**/*.h",
        "googletest/src/*.cc",
        "googlemock/src/*.cc",
    ]),
    includes = [
        "googlemock",
        "googlemock/include",
        "googletest",
        "googletest/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gtest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    copts = COPTS,
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [":gtest"],
)
