# dectris-compression
This library provides compression algorithms used by DECTRIS.

Supported algorithms (decompression only):

| Algorithm | Identifier | Description |
| --- | --- | --- |
| `COMPRESSION_BSLZ4` | `bslz4` | Bitshuffle with LZ4 compression (HDF5 framing) |
| `COMPRESSION_BSZSTD` | `bszstd` | Bitshuffle with Zstandard compression (HDF5 framing) |
| `COMPRESSION_LZ4` | `lz4` | LZ4 compression (HDF5 framing) |
| `COMPRESSION_TRPX` | `trpx` | TERSE/PROLIX v2 (TRPX) compression |

```python
from dectris.compression import decompress

decompress(data, "bszstd", elem_size=2)
decompress(data, "trpx")
```
