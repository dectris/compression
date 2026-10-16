# Patches for third party sources

The git submodules in `third_party/` are checked out at unmodified upstream
commits. Patches in this directory are applied at build time: by CMake and
`setup.py` (both with `apply_patches.py`) on a copy of the submodule sources,
and by Bazel on the upstream archive of the same commit
(`http_archive` in `MODULE.bazel`). Each patch file describes why it is needed.

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

Then update the `patches` of `trpx` in `MODULE.bazel`.
