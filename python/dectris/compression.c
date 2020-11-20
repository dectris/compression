#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "src/compression.h"

static int parse_algorithm(const char* identifier,
                           CompressionAlgorithm* algorithm) {
    if (strcmp(identifier, "bslz4-h5") == 0) {
        *algorithm = COMPRESSION_BSLZ4_HDF5;
        return 1;
    } else if (strcmp(identifier, "lz4-h5") == 0) {
        *algorithm = COMPRESSION_LZ4_HDF5;
        return 1;
    }
    return 0;
}

PyDoc_STRVAR(decompress__doc__, "\
Decompress, returning a bytes object of the uncompressed data.");

static PyObject* decompress(PyObject* module, PyObject* args, PyObject* kw) {
    static char* keywords[] = {"data", "algorithm", "elem_size", NULL};
    const char* input;
    Py_ssize_t input_size;
    const char* algorithm_str;
    Py_ssize_t algorithm_len;
    Py_ssize_t elem_size = 0;
    CompressionAlgorithm algorithm;
    size_t output_size;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "y#s#|$n", keywords, &input,
                                     &input_size, &algorithm_str,
                                     &algorithm_len, &elem_size))
        return NULL;

    if (!parse_algorithm(algorithm_str, &algorithm)) {
        PyErr_Format(PyExc_ValueError, "unknown algorithm: '%s'",
                     algorithm_str);
        return NULL;
    }

    if (elem_size < 0) {
        PyErr_SetString(PyExc_ValueError, "'elem_size' must be positive");
        return NULL;
    }

    if (algorithm == COMPRESSION_BSLZ4_HDF5) {
        if (elem_size == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "algorithm requires argument 'elem_size'");
            return NULL;
        }
    }

    output_size = compression_decompress_buffer(algorithm, NULL, 0, input,
                                                input_size, elem_size);
    if (output_size != COMPRESSION_ERROR) {
        PyObject* buffer = PyBytes_FromStringAndSize(NULL, output_size);
        if (!buffer)
            return NULL;

        if (compression_decompress_buffer(algorithm, PyBytes_AS_STRING(buffer),
                                          output_size, input, input_size,
                                          elem_size) == output_size)
            return buffer;

        Py_DECREF(buffer);
    }

    PyErr_SetString(PyExc_RuntimeError, "error decompressing stream");
    return NULL;
}

static PyMethodDef compression_methods[] = {
    {"decompress", (PyCFunction)&decompress, METH_VARARGS | METH_KEYWORDS,
     decompress__doc__},
    {NULL, NULL, 0, NULL},
};

PyDoc_STRVAR(compression__doc__, "\
This module provides an interface to compression algorithms used by DECTRIS.");

static PyModuleDef compression_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "compression",
    .m_doc = compression__doc__,
    .m_size = -1,
    .m_methods = compression_methods,
};

PyMODINIT_FUNC PyInit_compression(void) {
    return PyModule_Create(&compression_module);
}
