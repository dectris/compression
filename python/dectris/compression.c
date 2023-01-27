#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "src/compression.h"

static int parse_algorithm(const char* identifier,
                           CompressionAlgorithm* algorithm) {
    if (strcmp(identifier, "bslz4") == 0) {
        *algorithm = COMPRESSION_BSLZ4_HDF5;
        return 1;
    } else if (strcmp(identifier, "lz4") == 0) {
        *algorithm = COMPRESSION_LZ4_HDF5;
        return 1;
    }
    return 0;
}

PyDoc_STRVAR(decompress__doc__, "\
Decompress, returning a bytes object of the uncompressed data.");

static PyObject* decompress(PyObject* module, PyObject* args, PyObject* kw) {
    static char* keywords[] = {"data", "algorithm", "elem_size", NULL};
    Py_buffer input;
    const char* algorithm_str;
    Py_ssize_t algorithm_len;
    Py_ssize_t elem_size = 0;
    CompressionAlgorithm algorithm;
    size_t n;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "y*s#|$n", keywords, &input,
                                     &algorithm_str, &algorithm_len,
                                     &elem_size))
        return NULL;

    if (!parse_algorithm(algorithm_str, &algorithm)) {
        PyErr_Format(PyExc_ValueError, "unknown algorithm: '%s'",
                     algorithm_str);
        goto error;
    }

    if (elem_size < 0) {
        PyErr_SetString(PyExc_ValueError, "'elem_size' must be positive");
        goto error;
    }

    if (algorithm == COMPRESSION_BSLZ4_HDF5) {
        if (elem_size == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "algorithm requires argument 'elem_size'");
            goto error;
        }
    }

    n = compression_decompress_buffer(algorithm, NULL, 0, input.buf, input.len,
                                      elem_size);
    if (n != COMPRESSION_ERROR) {
        const size_t output_len = n;
        PyObject* buffer = PyBytes_FromStringAndSize(NULL, output_len);
        if (!buffer)
            goto error;

        Py_BEGIN_ALLOW_THREADS
        n = compression_decompress_buffer(algorithm, PyBytes_AS_STRING(buffer),
                                          output_len, input.buf, input.len,
                                          elem_size);
        Py_END_ALLOW_THREADS

        if (n == output_len) {
            PyBuffer_Release(&input);
            return buffer;
        }

        Py_DECREF(buffer);
    }

    PyErr_SetString(PyExc_RuntimeError, "error decompressing stream");
error:
    PyBuffer_Release(&input);
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
