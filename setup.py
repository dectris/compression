from glob import glob
import importlib.util
import os
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

ROOT = Path(__file__).parent
long_description = (ROOT / "README.md").read_text()

CXX_EXTENSIONS = (".cc", ".cpp", ".cxx")


def load_apply_patches():
    path = ROOT / "third_party" / "patches" / "apply_patches.py"
    spec = importlib.util.spec_from_file_location("apply_patches", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BuildExt(build_ext):
    def build_extensions(self):
        # Copy the TRPX headers (git submodule) to the build directory and
        # apply the patches in third_party/patches/trpx. See the description
        # in each patch file for why it is needed.
        trpx_dir = os.path.join(self.build_temp, "trpx")
        load_apply_patches().copy_and_patch(
            str(ROOT / "third_party" / "trpx"),
            trpx_dir,
            sorted(glob(str(ROOT / "third_party" / "patches" / "trpx" / "*.patch"))),
            paths=["include"],
        )
        for ext in self.extensions:
            ext.include_dirs.append(os.path.join(trpx_dir, "include"))

        if self.compiler.compiler_type == "msvc":
            for ext in self.extensions:
                ext.extra_compile_args.append("/std:c++20")
        else:
            # The C sources are C99, the C++ sources (TRPX) require C++20.
            compile_ = self.compiler._compile

            def _compile(obj, src, ext, cc_args, extra_postargs, pp_opts):
                std = "-std=c++20" if ext in CXX_EXTENSIONS else "-std=c99"
                compile_(obj, src, ext, cc_args, extra_postargs + [std], pp_opts)

            self.compiler._compile = _compile
            for ext in self.extensions:
                ext.extra_link_args.append("-pthread")
        build_ext.build_extensions(self)


setup(
    name="dectris-compression",
    version="0.3.1",
    description="DECTRIS compression algorithms",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Kal Conley",
    author_email="kal.conley@dectris.com",
    url="https://github.com/dectris/compression",
    package_dir={"": "python"},
    packages=["dectris"],
    ext_modules=[
        Extension(
            name="dectris.compression",
            sources=[
                "python/dectris/compression.c",
                "src/compression.c",
                "src/trpx.cc",
                "third_party/bitshuffle/src/bitshuffle.c",
                "third_party/lz4/lib/lz4.c",
            ]
            # zstd: decompression only.
            + sorted(glob("third_party/zstd/lib/common/*.c"))
            + sorted(glob("third_party/zstd/lib/decompress/*.c")),
            include_dirs=[
                ".",
                "third_party",
                "third_party/lz4/lib",
            ],
            define_macros=[
                # No legacy formats and no x86-64 assembly (portable C fallback).
                ("ZSTD_DISABLE_ASM", "1"),
                ("ZSTD_LEGACY_SUPPORT", "0"),
            ],
        )
    ],
    cmdclass={"build_ext": BuildExt},
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: C",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Programming Language :: Python :: 3.14",
        "Programming Language :: Python :: Implementation :: CPython",
    ],
    license="MIT",
    keywords=["compression", "dectris"],
)
