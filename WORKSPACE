load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "bitshuffle",
    path = "third_party/bitshuffle",
)

http_archive(
    name = "gtest",
    build_file = "//third_party:gtest.BUILD",
    sha256 = "78c676fc63881529bf97bf9d45948d905a66833fbfa5318ea2cd7478cb98f399",
    strip_prefix = "googletest-1.16.0",
    url = "https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz",
)

new_local_repository(
    name = "lz4",
    build_file = "//third_party:lz4.BUILD",
    path = "third_party/lz4",
)

new_local_repository(
    name = "zstd",
    build_file = "//third_party:zstd.BUILD",
    path = "third_party/zstd",
)

load("//third_party:trpx.bzl", "trpx_repository")

trpx_repository(name = "trpx")
