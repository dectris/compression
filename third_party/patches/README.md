# Patches for third party sources

The git submodules in `third_party/` are checked out at unmodified upstream
commits. Patches in this directory are applied on a copy of the sources at
build time, by Bazel (`patched_local_repository.bzl`), CMake (`patch`) and
`setup.py` (`apply_patches.py`). Each patch file describes why it is needed.

## trpx

Upstream: https://github.com/senikm/trpx2.0 (no LICENSE file upstream.)

The patches are generated with `git format-patch` so they apply in order at
exact line numbers. Some upstream files have CRLF line endings, which the
patches reproduce (hence `--keep-cr` below and `*.patch -text` in
`.gitattributes`). To change the patches, commit on top of the pinned
submodule commit and regenerate:

```sh
cd third_party/trpx
git checkout -b patches
git am --keep-cr ../patches/trpx/*.patch
# ... edit, commit ...
git format-patch --no-numbered --no-signature -o ../patches/trpx <pinned commit>..HEAD
git checkout --detach <pinned commit>
git branch -D patches
```

Then update `TRPX_PATCHES` in `third_party/trpx.bzl`.
