"""TERSE/PROLIX v2 (TRPX) header-only library.

The sources are the git submodule `third_party/trpx`. The patches in
`third_party/patches/trpx` are applied on top, see the description in each
patch file for why it is needed. Bazel, CMake and setup.py must apply the same
patches.
"""

load("//third_party:patched_local_repository.bzl", "patched_local_repository")

TRPX_HEADERS = [
    "include/Bitqueue.hpp",
    "include/Concurrent.hpp",
    "include/Shared_array.hpp",
    "include/Terse.hpp",
    "include/Unique_array.hpp",
    "include/XML_element.hpp",
]

TRPX_PATCHES = [
    "//third_party/patches/trpx:0001-Fix-narrowing-of-64-bit-sizes-on-32-bit-platforms.patch",
]

def trpx_repository(name = "trpx"):
    patched_local_repository(
        name = name,
        build_file = "//third_party:trpx.BUILD",
        files = TRPX_HEADERS,
        patches = TRPX_PATCHES,
        path = "third_party/trpx",
    )
