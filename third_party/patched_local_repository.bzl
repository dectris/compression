"""Repository rule for a local directory (e.g. a git submodule) with patches.

Unlike `new_local_repository` the files are copied into the external
repository instead of symlinked, so the patches can be applied without
modifying the source directory.
"""

def _patched_local_repository_impl(ctx):
    root = ctx.path(Label("@//:WORKSPACE")).dirname
    src = root.get_child(ctx.attr.path)
    for f in ctx.attr.files:
        # `read` and `file` with `legacy_utf8 = False` both use Latin-1, so
        # the file contents are copied byte for byte.
        ctx.file(f, ctx.read(src.get_child(f)), executable = False, legacy_utf8 = False)
    for patch in ctx.attr.patches:
        ctx.patch(patch, strip = ctx.attr.patch_strip)
    ctx.symlink(ctx.attr.build_file, "BUILD.bazel")
    ctx.file("WORKSPACE", "workspace(name = \"{}\")\n".format(ctx.name))

patched_local_repository = repository_rule(
    implementation = _patched_local_repository_impl,
    attrs = {
        "path": attr.string(
            mandatory = True,
            doc = "Source directory, relative to the workspace root.",
        ),
        "files": attr.string_list(
            mandatory = True,
            doc = "Files to copy from the source directory, relative to `path`.",
        ),
        "patches": attr.label_list(
            allow_files = True,
            doc = "Patches to apply in order after copying the files.",
        ),
        "patch_strip": attr.int(default = 1),
        "build_file": attr.label(
            mandatory = True,
            allow_single_file = True,
            doc = "BUILD file for the repository.",
        ),
    },
    local = True,
)
