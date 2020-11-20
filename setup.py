from setuptools import setup, Extension

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
            extra_compile_args=["-std=c99"],
        )
    ],
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
        "Programming Language :: Python :: Implementation :: CPython",
    ],
    license="MIT",
    keywords=["compression", "dectris"],
)
