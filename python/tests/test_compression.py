from dectris.compression import decompress
import pytest
import struct
import sys

# Streams produced by the DECTRIS champex encoder, see src/compression_test.cc.
BSZSTD_U8 = (
    b"\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x08\x00\x00\x00\x11\x28\xb5\x2f\xfd\x20\x08\x41\x00\x00\x55\xcc\x3c\x56\x98\xe0\x00\x00\x39\x40\x47\x4e",
    bytes([1, 8, 15, 22, 29, 36, 43, 50, 57, 64, 71, 78]),
)
BSZSTD_U16 = (
    b"\x00\x00\x00\x00\x00\x00\x00\x26\x00\x00\x00\x10\x00\x00\x00\x19\x28\xb5\x2f\xfd\x20\x10\x81\x00\x00\xff\x00\x55\xcc\x96\xb2\x24\x38\x6a\x4c\x70\x80\x00\x00\x00\x00\x00\x00\x00\x19\x28\xb5\x2f\xfd\x20\x10\x81\x00\x00\xff\x00\x55\xcc\x96\x4d\x69\x8e\xa5\x36\x38\x3f\xc0\x00\x00\x00\xc5\x12\xf1\x13\x1d\x15",
    struct.pack("<19H", *range(5, 5 + 300 * 19, 300)),
)
TRPX_U16 = (
    b'<Terse prolix_bits="16" signed="0" block="12" number_of_values="16" number_of_frames="1" memory_sizes_of_frames="48 " memory_size="48" metadata_string_sizes="0 "/>'
    b"\xfc\xff\xf3\x9b\x9f\x01\x40\xfa\x10\x7d\xe4\x2e\xa1\x0f\x00\x10\x77\x65\x6d\x41\x5f\xca\x18\x71\xe6\xab\x06\x77\x91\x65\x58\x1b\x53\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    struct.pack("<16H", *[65535 if i == 5 else i * 1000 for i in range(16)]),
)


def test_decompress_positional_args():
    assert (
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "lz4",
        )
        == b""
    )
    with pytest.raises(TypeError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "lz4",
            1,
        )
    if sys.version_info >= (3, 7):
        assert str(e.value) == "function takes at most 2 positional arguments (3 given)"
    else:
        assert str(e.value) == "Function takes at most 2 positional arguments (3 given)"


@pytest.mark.parametrize("algorithm", ["bslz4", "bszstd", "lz4"])
def test_decompress_keyword_args(algorithm):
    assert (
        decompress(
            data=b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm=algorithm,
            elem_size=1,
        )
        == b""
    )
    with pytest.raises(TypeError) as e:
        decompress(
            data=b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm=algorithm,
            zaphod=1,
        )
    # The error message of PyArg_ParseTupleAndKeywords changed in Python 3.13.
    assert str(e.value) in (
        "'zaphod' is an invalid keyword argument for this function",
        "this function got an unexpected keyword argument 'zaphod'",
    )


def test_decompress_bytes_like_data():
    assert (
        decompress(
            memoryview(b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"),
            "lz4",
        )
        == b""
    )


def test_decompress_mutable_data():
    assert (
        decompress(
            bytearray(b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"),
            "lz4",
        )
        == b""
    )


def test_decompress_non_bytes_like_data():
    with pytest.raises(TypeError) as e:
        decompress("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", "lz4")
    assert str(e.value) == "a bytes-like object is required, not 'str'"


@pytest.mark.parametrize(
    ("encoded", "kwargs"),
    [
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 1},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 2},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 4},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "lz4"},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bszstd", "elem_size": 1},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bszstd", "elem_size": 2},
        ),
    ],
)
def test_decompress_empty(encoded, kwargs):
    assert decompress(encoded, **kwargs) == b""


@pytest.mark.parametrize(
    ("encoded", "kwargs", "expected"),
    [
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x40\xf2\x1b\x33\xab\x22\x48\x27\xa9\x1a\x55\x40\xca\x40\xc0\xa5\x4a\x8c\x62\x01\xc0\xcf\xea\x4b\x86\xb8\x26\x2b\x71\x49\xe4\x00\x02\x88\xb3\x42\x0a\x00\x02\xec\x48\x30\x00\xfe\xff\x01\x00\xf0\x01\x7b\xfb\xe7\xee\xef\xef\xbe\xf7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09\x80\x53\xf2\xf9\xb8\x02\xff\x7b\x00",
            {"algorithm": "bslz4", "elem_size": 1},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x42\xf0\x31\x15\x80\x13\xf4\x88\x88\x83\x82\x81\x8b\x29\x24\xd1\xa9\x00\x50\x08\x00\x8a\x04\xfe\xff\xff\xff\xdd\xab\xbb\xf6\x00\x00\x00\x00\xf5\x25\xe5\x03\xb0\x80\x3c\x5a\x80\xfb\x93\x5e\x47\xc2\x10\xda\x31\x10\x2e\x04\xff\xff\xff\xff\xf7\xfd\xff\xdf\x00\x00\x00\x00\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "bslz4", "elem_size": 2},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x42\xf0\x31\x07\xe5\x00\x01\x11\x21\x1d\xc0\x00\x20\xfe\xff\x1f\xe5\x00\x00\x3f\x1b\x04\xc6\xd0\xe5\x8b\xc4\x45\x22\xff\xff\xff\xff\x00\x00\x80\xc1\xaa\x99\xb8\x46\xe8\x00\x02\x0b\xff\xff\xfa\xdf\x00\x00\x4c\x1c\x8c\x36\xf8\x39\x91\xb0\x04\x07\xff\xff\xed\xbf\x00\x00\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "bslz4", "elem_size": 4},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x40\x4d\x79\x20\x68\x61\x69\x72\x20\x69\x73\x20\x73\x68\x69\x6e\x67\x6c\x65\x64\x2c\x20\x61\x6e\x64\x20\x74\x68\x65\x20\x6c\x6f\x6e\x67\x65\x73\x74\x20\x73\x74\x72\x61\x6e\x64\x73\x20\x61\x72\x65\x20\x61\x62\x6f\x75\x74\x20\x6e\x69\x6e\x65\x20\x69\x6e\x63\x68\x00\x00\x00\x08\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "lz4"},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
    ],
)
def test_decompress_turing(encoded, kwargs, expected):
    assert decompress(encoded, **kwargs) == expected


@pytest.mark.parametrize(
    ("encoded", "kwargs", "expected"),
    [
        (BSZSTD_U8[0], {"algorithm": "bszstd", "elem_size": 1}, BSZSTD_U8[1]),
        (BSZSTD_U16[0], {"algorithm": "bszstd", "elem_size": 2}, BSZSTD_U16[1]),
        (TRPX_U16[0], {"algorithm": "trpx"}, TRPX_U16[1]),
        (TRPX_U16[0], {"algorithm": "trpx", "elem_size": 2}, TRPX_U16[1]),
        (TRPX_U16[0] * 2, {"algorithm": "trpx"}, TRPX_U16[1] * 2),
    ],
)
def test_decompress_golden(encoded, kwargs, expected):
    assert decompress(encoded, **kwargs) == expected


@pytest.mark.parametrize(
    ("encoded", "kwargs"),
    [
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 1},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 2},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4", "elem_size": 4},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "lz4"},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bszstd", "elem_size": 1},
        ),
        # Corrupted zstd frame.
        (
            BSZSTD_U16[0][:20] + bytes([BSZSTD_U16[0][20] ^ 0xFF]) + BSZSTD_U16[0][21:],
            {"algorithm": "bszstd", "elem_size": 2},
        ),
        # Wrong element size.
        (BSZSTD_U16[0], {"algorithm": "bszstd", "elem_size": 4}),
        (TRPX_U16[0], {"algorithm": "trpx", "elem_size": 4}),
        (TRPX_U16[0], {"algorithm": "trpx", "elem_size": 3}),
        # Not a TRPX stream, truncated stream, trailing garbage.
        (b"", {"algorithm": "trpx"}),
        (b"not a trpx stream", {"algorithm": "trpx"}),
        (TRPX_U16[0][:-8], {"algorithm": "trpx"}),
        (TRPX_U16[0] + b"garbage", {"algorithm": "trpx"}),
    ],
)
def test_decompress_error(encoded, kwargs):
    with pytest.raises(RuntimeError) as e:
        decompress(encoded, **kwargs)
    assert str(e.value) == "error decompressing stream"


@pytest.mark.parametrize(
    "algorithm",
    [
        "",
        "LZ4",
        "lz",
        "lz5",
        "lz4 ",
        " lz4",
    ],
)
def test_decompress_unknown_algorithm(algorithm):
    with pytest.raises(ValueError) as e:
        decompress(b"", algorithm)
    assert str(e.value) == f"unknown algorithm: '{algorithm}'"


@pytest.mark.parametrize("algorithm", ["bslz4", "bszstd", "lz4", "trpx"])
@pytest.mark.parametrize("elem_size", [-1, -32768])
def test_decompress_elem_size_must_be_positive(algorithm, elem_size):
    with pytest.raises(ValueError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm,
            elem_size=elem_size,
        )
    assert str(e.value) == "'elem_size' must be positive"


@pytest.mark.parametrize("algorithm", ["bslz4", "bszstd"])
def test_decompress_algorithm_requires_elem_size(algorithm):
    with pytest.raises(ValueError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm,
        )
    assert str(e.value) == "algorithm requires argument 'elem_size'"
