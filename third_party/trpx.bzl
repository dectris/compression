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
    "//third_party/patches/trpx:0001-Add-missing-standard-library-includes.patch",
    "//third_party/patches/trpx:0002-Create-the-global-thread-pool-lazily.patch",
    "//third_party/patches/trpx:0003-Fix-unused-variable-warning.patch",
    "//third_party/patches/trpx:0004-Fix-heap-buffer-overflow-when-growing-the-compressed.patch",
    "//third_party/patches/trpx:0005-Fix-corrupt-output-for-32-bit-data-with-saturated-pi.patch",
    "//third_party/patches/trpx:0006-Fix-decoder-reading-past-the-end-of-a-frame-for-a-sh.patch",
    "//third_party/patches/trpx:0007-Fix-reading-frame-metadata-strings-into-the-std-stri.patch",
    "//third_party/patches/trpx:0008-Add-typename-to-dependent-type-names.patch",
    "//third_party/patches/trpx:0009-Do-not-use-std-make_shared_for_overwrite.patch",
    "//third_party/patches/trpx:0010-Fix-narrowing-of-64-bit-sizes-on-32-bit-platforms.patch",
]

def trpx_repository(name = "trpx"):
    patched_local_repository(
        name = name,
        build_file = "//third_party:trpx.BUILD",
        files = TRPX_HEADERS,
        patches = TRPX_PATCHES,
        path = "third_party/trpx",
    )
