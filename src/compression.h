#ifndef DECTRIS_COMPRESSION_H_
#define DECTRIS_COMPRESSION_H_

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define COMPRESSION_ERROR SIZE_MAX

/* DECTRIS compression algorithms */
typedef enum {
    /* Bitshuffle with LZ4 compression (HDF5 framing)
     *
     * Data is stored as a series of bitshuffle transposed blocks compressed
     * with LZ4. The format is the same as the bitshuffle HDF5 filter.
     *
     * # See also
     *
     * https://github.com/kiyo-masui/bitshuffle
     * https://github.com/kiyo-masui/bitshuffle/blob/f57e2e50cc6855d5cf7689b9bc688b3ef1dffe02/src/bshuf_h5filter.c
     * https://lz4.github.io/lz4/
     * https://github.com/lz4/lz4/blob/master/doc/lz4_Block_format.md
     */
    COMPRESSION_BSLZ4,

    /* LZ4 compression (HDF5 framing)
     *
     * Data is stored as a series of LZ4 compressed blocks. The LZ4 filter
     * format for HDF5 is used for framing.
     *
     * # See also
     *
     * https://lz4.github.io/lz4/
     * https://github.com/lz4/lz4/blob/master/doc/lz4_Block_format.md
     * https://support.hdfgroup.org/services/filters/HDF5_LZ4.pdf
     */
    COMPRESSION_LZ4,

    /* Bitshuffle with Zstandard compression (HDF5 framing)
     *
     * Data is stored as a series of bitshuffle transposed blocks each
     * compressed as a Zstandard frame. The format is the same as the
     * bitshuffle HDF5 filter with BSHUF_H5_COMPRESS_ZSTD.
     *
     * # See also
     *
     * https://github.com/kiyo-masui/bitshuffle
     * https://github.com/kiyo-masui/bitshuffle/blob/0.5.2/src/bshuf_h5filter.c
     * https://facebook.github.io/zstd/
     */
    COMPRESSION_BSZSTD,

    /* TERSE/PROLIX v2 (TRPX) compression
     *
     * Data is stored as one or more concatenated TRPX streams, each
     * consisting of an XML header followed by one or more compressed frames.
     * All frames are decompressed consecutively. The format is the same as
     * the TERSE HDF5 filter (id 32029).
     *
     * Only unsigned 8, 16 and 32 bit elements are supported. `elem_size` may
     * be 0, in which case the element size of the stream is used. Otherwise
     * it must match the element size of the stream.
     *
     * # See also
     *
     * https://github.com/senikm/trpx2.0
     */
    COMPRESSION_TRPX,

} CompressionAlgorithm;

/* Decompresses the contents of a source buffer into a destination buffer.
 *
 * `elem_size` is the size in bytes of one element of the uncompressed data.
 * It is required by COMPRESSION_BSLZ4 and COMPRESSION_BSZSTD, optional for
 * COMPRESSION_TRPX and ignored by COMPRESSION_LZ4.
 *
 * If `dst_size` is 0, no data is written and the size of the uncompressed data
 * is returned. Otherwise the uncompressed data is written to `dst` and its size
 * is returned. Returns COMPRESSION_ERROR on failure. */
size_t compression_decompress_buffer(CompressionAlgorithm algorithm,
                                     char* dst,
                                     size_t dst_size,
                                     const char* src,
                                     size_t src_size,
                                     size_t elem_size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* DECTRIS_COMPRESSION_H_ */
