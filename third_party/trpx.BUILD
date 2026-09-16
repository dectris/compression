load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# `Terse.hpp` requires `Concurrent.hpp` to be included first, also for the
# single threaded `jpa::Terse<void>`:
#
#     #include "Concurrent.hpp"
#     #include "Terse.hpp"
#
# `includes` adds the directory as a system include path for dependents so the
# third party headers do not trigger their warnings.
cc_library(
    name = "trpx",
    hdrs = glob(["include/*.hpp"]),
    includes = ["include"],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
)
