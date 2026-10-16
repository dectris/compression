load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# `Terse.hpp` requires `Concurrent.hpp` to be included first, also for the
# single threaded `jpa::Terse<void>`:
#
#     #include "Concurrent.hpp"
#     #include "Terse.hpp"
#
# The headers are external includes for dependents so the third party headers
# do not trigger their warnings (-isystem, MSVC: /external:I with
# /external:W0).
cc_library(
    name = "trpx",
    hdrs = glob(["include/*.hpp"]),
    features = ["external_include_paths"],
    includes = ["include"],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
)
