# dectris-compression
This library provides compression algorithms used by DECTRIS.

Supported algorithms:

| Algorithm | Identifier | Description |
| --- | --- | --- |
| `COMPRESSION_BSLZ4` | `bslz4` | Bitshuffle with LZ4 compression (HDF5 framing) |
| `COMPRESSION_BSZSTD` | `bszstd` | Bitshuffle with Zstandard compression (HDF5 framing) |
| `COMPRESSION_LZ4` | `lz4` | LZ4 compression (HDF5 framing) |
| `COMPRESSION_TRPX` | `trpx` | TERSE/PROLIX v2 (TRPX) compression |

All algorithms support decompression. The C API also supports compression
with `COMPRESSION_BSZSTD` and `COMPRESSION_TRPX` (`compression_compress_buffer()`)
and block by block streaming compression with `COMPRESSION_BSZSTD`
(`compression_stream_encoder_create()`).

```python
from dectris.compression import decompress

decompress(data, "bszstd", elem_size=2)
decompress(data, "trpx")
```

## Bazel

The repository is a Bazel module (`dectris_compression`). It does not use the
git submodules, so a source archive of a release can be used directly:

```starlark
# MODULE.bazel
bazel_dep(name = "dectris_compression", version = "<version>")
archive_override(
    module_name = "dectris_compression",
    integrity = "sha256-...",
    strip_prefix = "compression-<version>",
    urls = ["https://github.com/dectris/compression/archive/refs/tags/v<version>.tar.gz"],
)
```

```starlark
# BUILD.bazel
cc_library(
    ...
    deps = ["@dectris_compression//:compression"],
)
```

```c
#include "dectris_compression/src/compression.h"
```
