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

/* Returns an upper bound of the size of the compressed data of `src_size`
 * bytes of uncompressed data, i.e. a destination buffer of this size is large
 * enough for compression_compress_buffer().
 *
 * Returns COMPRESSION_ERROR if the algorithm does not support compression or
 * the arguments are invalid (see compression_compress_buffer()). */
size_t compression_compress_bound(CompressionAlgorithm algorithm,
                                  size_t src_size,
                                  size_t elem_size);

/* Compresses the contents of a source buffer into a destination buffer.
 *
 * Supported algorithms are COMPRESSION_BSZSTD and COMPRESSION_TRPX.
 * `elem_size` is the size in bytes of one element of the uncompressed data.
 * `src_size` must be a multiple of it.
 *
 * COMPRESSION_BSZSTD uses the default block size of the bitshuffle HDF5
 * filter (8192 bytes for elements of up to 64 bytes) and Zstandard
 * compression level 1.
 *
 * COMPRESSION_TRPX writes the data as a single TRPX stream with one frame.
 * Only unsigned 8, 16 and 32 bit elements (`elem_size` 1, 2 or 4) are
 * supported.
 *
 * Returns the size of the compressed data written to `dst`. Returns
 * COMPRESSION_ERROR on failure, including if `dst_size` is too small (see
 * compression_compress_bound()). */
size_t compression_compress_buffer(CompressionAlgorithm algorithm,
                                   char* dst,
                                   size_t dst_size,
                                   const char* src,
                                   size_t src_size,
                                   size_t elem_size);

/* Streaming compression, block by block.
 *
 * A stream encoder produces the same format as compression_compress_buffer()
 * but compresses the data in blocks as it is provided, e.g. while it is
 * produced. The encoder (its buffers and compression context) is reused for
 * all blocks and for further streams.
 *
 * Only COMPRESSION_BSZSTD is supported. `block_size` is the size in bytes of a
 * block of uncompressed data. It must be a multiple of 8 elements and at most
 * 0x7E000000 bytes. 0 selects the default block size of the bitshuffle HDF5
 * filter (as used by compression_compress_buffer()).
 *
 * Usage:
 *
 *     encoder = compression_stream_encoder_create(algorithm, block_size,
 *                                                 elem_size);
 *     n = compression_stream_encoder_start(encoder, dst, dst_size, src_size);
 *     while (data left)
 *         n += compression_stream_encoder_block(encoder, dst + n,
 *                                               dst_size - n, block,
 *                                               block_bytes);
 *     compression_stream_encoder_free(encoder);
 */
typedef struct CompressionStreamEncoder CompressionStreamEncoder;

/* Returns an upper bound of the size of the compressed data of a stream of
 * `src_size` bytes of uncompressed data, or COMPRESSION_ERROR if the
 * algorithm does not support streaming compression or the arguments are
 * invalid. */
size_t compression_stream_bound(CompressionAlgorithm algorithm,
                                size_t src_size,
                                size_t block_size,
                                size_t elem_size);

/* Creates a stream encoder. Returns NULL if the algorithm does not support
 * streaming compression, the arguments are invalid or on allocation failure.
 */
CompressionStreamEncoder* compression_stream_encoder_create(
    CompressionAlgorithm algorithm,
    size_t block_size,
    size_t elem_size);

/* Frees a stream encoder. NULL is ignored. */
void compression_stream_encoder_free(CompressionStreamEncoder* encoder);

/* Starts a stream of `src_size` bytes of uncompressed data, discarding any
 * stream in progress. Writes the stream header to `dst` and returns its size,
 * or COMPRESSION_ERROR on failure. */
size_t compression_stream_encoder_start(CompressionStreamEncoder* encoder,
                                        char* dst,
                                        size_t dst_size,
                                        size_t src_size);

/* Compresses the next block of the stream into `dst`. `src_size` must be the
 * block size, except for the last block of the stream, which holds the
 * remaining data. Returns the number of bytes written to `dst`, or
 * COMPRESSION_ERROR on failure (including if `dst_size` is too small, see
 * compression_stream_bound()). After a failure the stream must be started
 * again. */
size_t compression_stream_encoder_block(CompressionStreamEncoder* encoder,
                                        char* dst,
                                        size_t dst_size,
                                        const char* src,
                                        size_t src_size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* DECTRIS_COMPRESSION_H_ */
