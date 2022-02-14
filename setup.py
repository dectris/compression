from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class BuildExt(build_ext):
    def build_extensions(self):
        if self.compiler.compiler_type != "msvc":
            for ext in self.extensions:
                ext.extra_compile_args.append("-std=c99")
        build_ext.build_extensions(self)


setup(
    name="dectris-compression",
    version="0.0.0",
    description="DECTRIS compression algorithms",
    author="Kal Conley",
    author_email="kal.conley@dectris.com",
    package_dir={"": "python"},
    packages=["dectris"],
    url="https://github.com/dectris/compression",
    ext_modules=[
        Extension(
            name="dectris.compression",
            sources=[
                "python/dectris/compression.c",
                "src/compression.c",
                "third_party/bitshuffle/src/bitshuffle_core.c",
                "third_party/bitshuffle/src/iochain.c",
                "third_party/lz4/lib/lz4.c",
            ],
            include_dirs=[
                ".",
                "third_party",
                "third_party/lz4/lib",
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
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: Implementation :: CPython",
    ],
    license="MIT",
    keywords=["compression", "dectris"],
)
