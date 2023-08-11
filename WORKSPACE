load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "bitshuffle",
    path = "third_party/bitshuffle",
)

http_archive(
    name = "gtest",
    build_file = "//third_party:gtest.BUILD",
    sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
    strip_prefix = "googletest-release-1.10.0",
    url = "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
)

new_local_repository(
    name = "lz4",
    build_file = "//third_party:lz4.BUILD",
    path = "third_party/lz4",
)
