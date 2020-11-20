from dectris.compression import decompress
import pytest
import sys


def test_decompress_positional_args():
    assert (
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "lz4-h5",
        )
        == b""
    )
    with pytest.raises(TypeError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "lz4-h5",
            1,
        )
    if sys.version_info >= (3, 7):
        assert str(e.value) == "function takes at most 2 positional arguments (3 given)"
    else:
        assert str(e.value) == "Function takes at most 2 positional arguments (3 given)"


@pytest.mark.parametrize("algorithm", ["bslz4-h5", "lz4-h5"])
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
    assert str(e.value) == "'zaphod' is an invalid keyword argument for this function"


@pytest.mark.parametrize(
    ("encoded", "kwargs"),
    [
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 1},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 2},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 4},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            {"algorithm": "lz4-h5"},
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
            {"algorithm": "bslz4-h5", "elem_size": 1},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x42\xf0\x31\x15\x80\x13\xf4\x88\x88\x83\x82\x81\x8b\x29\x24\xd1\xa9\x00\x50\x08\x00\x8a\x04\xfe\xff\xff\xff\xdd\xab\xbb\xf6\x00\x00\x00\x00\xf5\x25\xe5\x03\xb0\x80\x3c\x5a\x80\xfb\x93\x5e\x47\xc2\x10\xda\x31\x10\x2e\x04\xff\xff\xff\xff\xf7\xfd\xff\xdf\x00\x00\x00\x00\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "bslz4-h5", "elem_size": 2},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x42\xf0\x31\x07\xe5\x00\x01\x11\x21\x1d\xc0\x00\x20\xfe\xff\x1f\xe5\x00\x00\x3f\x1b\x04\xc6\xd0\xe5\x8b\xc4\x45\x22\xff\xff\xff\xff\x00\x00\x80\xc1\xaa\x99\xb8\x46\xe8\x00\x02\x0b\xff\xff\xfa\xdf\x00\x00\x4c\x1c\x8c\x36\xf8\x39\x91\xb0\x04\x07\xff\xff\xed\xbf\x00\x00\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "bslz4-h5", "elem_size": 4},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x40\x00\x00\x00\x40\x4d\x79\x20\x68\x61\x69\x72\x20\x69\x73\x20\x73\x68\x69\x6e\x67\x6c\x65\x64\x2c\x20\x61\x6e\x64\x20\x74\x68\x65\x20\x6c\x6f\x6e\x67\x65\x73\x74\x20\x73\x74\x72\x61\x6e\x64\x73\x20\x61\x72\x65\x20\x61\x62\x6f\x75\x74\x20\x6e\x69\x6e\x65\x20\x69\x6e\x63\x68\x00\x00\x00\x08\x65\x73\x20\x6c\x6f\x6e\x67\x2e",
            {"algorithm": "lz4-h5"},
            b"My hair is shingled, and the longest strands are about nine inches long.",
        ),
    ],
)
def test_decompress_turing(encoded, kwargs, expected):
    assert decompress(encoded, **kwargs) == expected


@pytest.mark.parametrize(
    ("encoded", "kwargs"),
    [
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 1},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 2},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "bslz4-h5", "elem_size": 4},
        ),
        (
            b"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00",
            {"algorithm": "lz4-h5"},
        ),
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
        "LZ4-H5",
        "lz4-h",
        "lz4-h4",
        "lz4-h5 ",
        " lz4-h5",
    ],
)
def test_decompress_unknown_algorithm(algorithm):
    with pytest.raises(ValueError) as e:
        decompress(b"", algorithm)
    assert str(e.value) == f"unknown algorithm: '{algorithm}'"


@pytest.mark.parametrize("algorithm", ["bslz4-h5", "lz4-h5"])
@pytest.mark.parametrize("elem_size", [-1, -32768])
def test_decompress_elem_size_must_be_positive(algorithm, elem_size):
    with pytest.raises(ValueError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm,
            elem_size=elem_size,
        )
    assert str(e.value) == "'elem_size' must be positive"


@pytest.mark.parametrize("algorithm", ["bslz4-h5"])
def test_decompress_algorithm_requires_elem_size(algorithm):
    with pytest.raises(ValueError) as e:
        decompress(
            b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            algorithm,
        )
    assert str(e.value) == "algorithm requires argument 'elem_size'"
