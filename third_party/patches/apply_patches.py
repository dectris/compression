"""Copy a directory and apply unified diff patches to the copy.

A minimal, strict patch tool for the patches in this directory, which are
generated with `git format-patch` against pinned git submodule commits: hunks
are applied at the recorded line numbers (no fuzz) and all context and removed
lines must match. Line endings (LF or CRLF) are compared and preserved as
present in the file, so the patches also apply after git autocrlf conversion.

Usage: apply_patches.py [--copy PATH]... SRC_DIR DST_DIR PATCH...

Copies the given paths (files or directories, relative to SRC_DIR; default: all
of SRC_DIR) to DST_DIR, replacing DST_DIR, and applies the patches (`-p1`) with
DST_DIR as the root.
"""

import argparse
import os
import re
import shutil
import sys

_HUNK_HEADER = re.compile(rb"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


class PatchError(Exception):
    pass


def _parse_patch(patch_path, strip=1):
    """Yields (path, hunks) with hunks = [(old_start, lines)]."""
    with open(patch_path, "rb") as f:
        lines = f.read().split(b"\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith(b"--- "):
            i += 1
            continue
        if i + 1 >= len(lines) or not lines[i + 1].startswith(b"+++ "):
            raise PatchError(f"{patch_path}: malformed file header at line {i + 1}")
        new_path = lines[i + 1][4:].split(b"\t")[0].rstrip(b"\r").decode()
        if new_path == "/dev/null":
            raise PatchError(f"{patch_path}: file deletions are not supported")
        path = "/".join(new_path.split("/")[strip:])
        i += 2
        hunks = []
        while i < len(lines):
            match = _HUNK_HEADER.match(lines[i])
            if not match:
                break
            old_start = int(match.group(1))
            old_len = int(match.group(2) or 1)
            new_len = int(match.group(4) or 1)
            i += 1
            hunk = []
            while old_len > 0 or new_len > 0:
                if i >= len(lines):
                    raise PatchError(f"{patch_path}: truncated hunk")
                hunk_line = lines[i]
                i += 1
                if hunk_line.startswith(b"\\"):  # "\ No newline at end of file"
                    raise PatchError(f"{patch_path}: files without trailing newline are not supported")
                tag = hunk_line[:1]
                if tag == b" ":
                    old_len -= 1
                    new_len -= 1
                elif tag == b"-":
                    old_len -= 1
                elif tag == b"+":
                    new_len -= 1
                else:
                    raise PatchError(f"{patch_path}: malformed hunk line {i}: {hunk_line!r}")
                hunk.append(hunk_line)
            hunks.append((old_start, hunk))
        if not hunks:
            raise PatchError(f"{patch_path}: no hunks for {path}")
        yield path, hunks


def _apply_hunks(path, hunks, patch_path):
    with open(path, "rb") as f:
        content = f.read()
    if not content.endswith(b"\n"):
        raise PatchError(f"{patch_path}: {path} has no trailing newline")
    old_lines = content[:-1].split(b"\n")
    newline = b"\r\n" if old_lines[0].endswith(b"\r") else b"\n"

    new_lines = []
    pos = 0  # next unconsumed line of old_lines
    for old_start, hunk in hunks:
        start = old_start - 1 if hunk and any(l[:1] != b"+" for l in hunk) else old_start
        if start < pos:
            raise PatchError(f"{patch_path}: overlapping hunks in {path}")
        new_lines.extend(old_lines[pos:start])
        pos = start
        for hunk_line in hunk:
            tag, text = hunk_line[:1], hunk_line[1:].rstrip(b"\r")
            if tag in (b" ", b"-"):
                if pos >= len(old_lines) or old_lines[pos].rstrip(b"\r") != text:
                    actual = old_lines[pos] if pos < len(old_lines) else b"<EOF>"
                    raise PatchError(
                        f"{patch_path}: hunk @@ -{old_start} @@ does not apply to {path} "
                        f"at line {pos + 1}: expected {text!r}, found {actual!r}"
                    )
                pos += 1
            if tag in (b" ", b"+"):
                new_lines.append(text + newline[:-1])
    new_lines.extend(old_lines[pos:])

    with open(path, "wb") as f:
        f.write(b"\n".join(new_lines) + b"\n")


def apply_patch(patch_path, root, strip=1):
    for path, hunks in _parse_patch(patch_path, strip):
        target = os.path.join(root, *path.split("/"))
        if not os.path.isfile(target):
            raise PatchError(f"{patch_path}: {target} does not exist")
        _apply_hunks(target, hunks, patch_path)


def copy_and_patch(src_dir, dst_dir, patches, paths=("",)):
    """Copies `paths` of `src_dir` to `dst_dir` (replacing it), applies `patches`."""
    if os.path.exists(dst_dir):
        shutil.rmtree(dst_dir)
    for path in paths:
        src = os.path.join(src_dir, path)
        dst = os.path.join(dst_dir, path)
        if os.path.isdir(src):
            shutil.copytree(src, dst)
        else:
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)
    for patch in patches:
        apply_patch(patch, dst_dir)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--copy", action="append", default=[], metavar="PATH")
    parser.add_argument("src_dir")
    parser.add_argument("dst_dir")
    parser.add_argument("patches", nargs="+", metavar="PATCH")
    args = parser.parse_args(argv)
    try:
        copy_and_patch(args.src_dir, args.dst_dir, args.patches, args.copy or ("",))
    except PatchError as e:
        sys.exit(f"error: {e}")


if __name__ == "__main__":
    main()
