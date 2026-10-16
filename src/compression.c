#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#elif defined(_WIN32)
#define INCL_EXTRA_HTON_FUNCTIONS 1
#include <winsock2.h>
#define be32toh(x) ntohl(x)
#define be64toh(x) ntohll(x)
#define htobe32(x) htonl(x)
#define htobe64(x) htonll(x)
#pragma comment(lib, "ws2_32.lib")
#else
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <endian.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "bitshuffle.h"
#include "lz4.h"
#include "zstd.h"

#include "src/compression.h"
#include "src/trpx.h"

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#ifndef BSHUF_BLOCKED_MULT
#define BSHUF_BLOCKED_MULT 8
#endif

/* Default block size of bitshuffle, see bshuf_default_block_size(). */
#define BSHUF_TARGET_BLOCK_SIZE_B 8192
#define BSHUF_MIN_RECOMMEND_BLOCK 128

/* Compression level of the Zstandard compressed blocks of
 * COMPRESSION_BSZSTD. The same default as the bitshuffle Zstandard API;
 * higher levels are considerably slower for little gain on bit-shuffled
 * detector data. */
#define BSZSTD_COMPRESSION_LEVEL 1

static uint32_t read_u32_be(const char* buf) {
    uint32_t value;
    memcpy(&value, buf, sizeof(value));
    return be32toh(value);
}

static uint64_t read_u64_be(const char* buf) {
    uint64_t value;
    memcpy(&value, buf, sizeof(value));
    return be64toh(value);
}

static void write_u32_be(char* buf, uint32_t value) {
    value = htobe32(value);
    memcpy(buf, &value, sizeof(value));
}

static void write_u64_be(char* buf, uint64_t value) {
    value = htobe64(value);
    memcpy(buf, &value, sizeof(value));
}

static _Bool parse_header(const char** const src,
                          const char* src_end,
                          uint64_t* const orig_size,
                          uint32_t* const block_size) {
    if (src_end - *src < 12)
        return 0;
    *orig_size = read_u64_be(*src);
    *block_size = read_u32_be(*src + 8);
    if (*orig_size >= SIZE_MAX || *block_size >= SIZE_MAX)
        return 0;
    *src += 12;
    return 1;
}

/* Compression applied to the individual bitshuffle transposed blocks. */
typedef enum {
    BLOCK_CODEC_LZ4,
    BLOCK_CODEC_ZSTD,
} BlockCodec;

static _Bool decompress_block(BlockCodec codec,
                              char* const dst,
                              const uint32_t dst_size,
                              const char* const src,
                              const uint32_t src_size) {
    switch (codec) {
        case BLOCK_CODEC_LZ4:
            return LZ4_decompress_safe(src, dst, src_size, dst_size) ==
                   (int)dst_size;
        case BLOCK_CODEC_ZSTD: {
            const size_t n = ZSTD_decompress(dst, dst_size, src, src_size);
            return !ZSTD_isError(n) && n == dst_size;
        }
    }
    return 0;
}

static _Bool decompress_bshuf_block(BlockCodec codec,
                                    char** const dst,
                                    const char** const src,
                                    const char* const src_end,
                                    char* const tmp_buf,
                                    const uint32_t block_size,
                                    const size_t elem_size) {
    if (src_end - *src < 4)
        return 0;
    const uint32_t compressed_size = read_u32_be(*src);
    *src += 4;
    if (compressed_size > INT_MAX || (int)compressed_size > src_end - *src)
        return 0;
    if (!decompress_block(codec, &tmp_buf[0], block_size, *src,
                          compressed_size))
    {
        return 0;
    }
    if (bitshuf_decode_block(*dst, &tmp_buf[0], &tmp_buf[block_size],
                             block_size / elem_size, elem_size) != 0)
    {
        return 0;
    }
    *dst += block_size;
    *src += compressed_size;
    return 1;
}

static size_t decompress_buffer_bshuf_hdf5(BlockCodec codec,
                                           char* dst,
                                           size_t dst_size,
                                           const char* src,
                                           size_t src_size,
                                           size_t elem_size) {
    if (elem_size == 0 || elem_size > UINT32_MAX / BSHUF_BLOCKED_MULT)
        return COMPRESSION_ERROR;

    const char* const src_end = src + src_size;
    uint64_t orig_size;
    uint32_t block_size;
    if (!parse_header(&src, src_end, &orig_size, &block_size))
        return COMPRESSION_ERROR;

    if (dst_size == 0)
        return (size_t)orig_size;

    if (orig_size > dst_size || (orig_size != 0 && block_size == 0) ||
        block_size % (BSHUF_BLOCKED_MULT * elem_size) != 0 ||
        block_size > INT_MAX)
    {
        return COMPRESSION_ERROR;
    }

    if (orig_size == 0)
        return 0;

    char* tmp_buf = malloc(block_size * 2);
    if (!tmp_buf)
        return COMPRESSION_ERROR;

    const char* const dst_end = dst + dst_size;
    const int leftover_bytes =
        orig_size % (int)(BSHUF_BLOCKED_MULT * elem_size);
    const uint64_t full_block_count = orig_size / block_size;
    const uint32_t last_block_size = (orig_size % block_size) - leftover_bytes;

    for (uint64_t block = 0; block < full_block_count; ++block) {
        if (!decompress_bshuf_block(codec, &dst, &src, src_end, tmp_buf,
                                    block_size, elem_size))
        {
            free(tmp_buf);
            return COMPRESSION_ERROR;
        }
    }
    if (last_block_size > 0) {
        if (!decompress_bshuf_block(codec, &dst, &src, src_end, tmp_buf,
                                    last_block_size, elem_size))
        {
            free(tmp_buf);
            return COMPRESSION_ERROR;
        }
    }
    free(tmp_buf);

    if (leftover_bytes > 0) {
        if (leftover_bytes > dst_end - dst || leftover_bytes != src_end - src)
            return COMPRESSION_ERROR;
        memcpy(dst, src, leftover_bytes);
    }

    return (size_t)orig_size;
}

static _Bool decompress_lz4_block(char** const dst,
                                  const char** const src,
                                  const char* const src_end,
                                  const uint32_t block_size) {
    if (src_end - *src < 4)
        return 0;
    const uint32_t compressed_size = read_u32_be(*src);
    *src += 4;
    if (compressed_size > INT_MAX || (int)compressed_size > src_end - *src)
        return 0;
    if (compressed_size == block_size) {
        memcpy(*dst, *src, block_size);
    } else {
        if (LZ4_decompress_safe(*src, *dst, compressed_size, block_size) !=
            (int)block_size)
        {
            return 0;
        }
    }
    *dst += block_size;
    *src += compressed_size;
    return 1;
}

static size_t decompress_buffer_lz4_hdf5(char* dst,
                                         size_t dst_size,
                                         const char* src,
                                         size_t src_size) {
    const char* const src_end = src + src_size;
    uint64_t orig_size;
    uint32_t block_size;
    if (!parse_header(&src, src_end, &orig_size, &block_size))
        return COMPRESSION_ERROR;

    if (dst_size == 0)
        return (size_t)orig_size;

    if (orig_size > dst_size || (orig_size != 0 && block_size == 0) ||
        block_size > INT_MAX)
    {
        return COMPRESSION_ERROR;
    }

    if (orig_size == 0)
        return 0;

    const uint64_t full_block_count = orig_size / block_size;
    const uint32_t last_block_size = orig_size % block_size;

    for (uint64_t block = 0; block < full_block_count; ++block) {
        if (!decompress_lz4_block(&dst, &src, src_end, block_size))
            return COMPRESSION_ERROR;
    }
    if (last_block_size > 0) {
        if (!decompress_lz4_block(&dst, &src, src_end, last_block_size))
            return COMPRESSION_ERROR;
    }

    return (size_t)orig_size;
}

size_t compression_decompress_buffer(CompressionAlgorithm algorithm,
                                     char* dst,
                                     size_t dst_size,
                                     const char* src,
                                     size_t src_size,
                                     size_t elem_size) {
    switch (algorithm) {
        case COMPRESSION_BSLZ4:
            return decompress_buffer_bshuf_hdf5(BLOCK_CODEC_LZ4, dst, dst_size,
                                                src, src_size, elem_size);
        case COMPRESSION_LZ4:
            return decompress_buffer_lz4_hdf5(dst, dst_size, src, src_size);
        case COMPRESSION_BSZSTD:
            return decompress_buffer_bshuf_hdf5(BLOCK_CODEC_ZSTD, dst, dst_size,
                                                src, src_size, elem_size);
        case COMPRESSION_TRPX:
            return compression_decompress_buffer_trpx(dst, dst_size, src,
                                                      src_size, elem_size);
    }
    return COMPRESSION_ERROR;
}

/* Largest block size in bytes of COMPRESSION_BSZSTD. The compressed size of a
 * block (ZSTD_compressBound()) must fit the signed 32 bit block header read by
 * the bitshuffle HDF5 filter. The same limit as LZ4_MAX_INPUT_SIZE. */
#define BSZSTD_MAX_BLOCK_SIZE 0x7E000000

/* Returns the block size in bytes of COMPRESSION_BSZSTD for the requested
 * `block_size` in bytes (0 selects the default block size of the bitshuffle
 * HDF5 filter, see bshuf_default_block_size()), or 0 if `block_size` or
 * `elem_size` is not supported. */
static uint32_t bszstd_block_size(size_t block_size, size_t elem_size) {
    if (elem_size == 0 || elem_size > BSZSTD_MAX_BLOCK_SIZE / BSHUF_MIN_RECOMMEND_BLOCK)
        return 0;
    if (block_size == 0) {
        size_t block_elems = BSHUF_TARGET_BLOCK_SIZE_B / elem_size;
        block_elems = (block_elems / BSHUF_BLOCKED_MULT) * BSHUF_BLOCKED_MULT;
        if (block_elems < BSHUF_MIN_RECOMMEND_BLOCK)
            block_elems = BSHUF_MIN_RECOMMEND_BLOCK;
        block_size = block_elems * elem_size;
    }
    if (block_size % (BSHUF_BLOCKED_MULT * elem_size) != 0 ||
        block_size > BSZSTD_MAX_BLOCK_SIZE)
    {
        return 0;
    }
    return (uint32_t)block_size;
}

static size_t stream_bound_bszstd(size_t src_size,
                                  size_t block_size,
                                  size_t elem_size) {
    block_size = bszstd_block_size(block_size, elem_size);
    if (block_size == 0 || src_size % elem_size != 0)
        return COMPRESSION_ERROR;
    /* Guards the computation below against overflows. */
    if (src_size > SIZE_MAX / 2)
        return COMPRESSION_ERROR;

    const size_t leftover_bytes = src_size % (BSHUF_BLOCKED_MULT * elem_size);
    const size_t full_block_count = src_size / block_size;
    const size_t last_block_size = src_size % block_size - leftover_bytes;

    /* The stream header is 12 bytes and each block has a 4 byte header. */
    size_t bound = 12 + leftover_bytes;
    bound += full_block_count * (4 + ZSTD_compressBound(block_size));
    if (last_block_size > 0)
        bound += 4 + ZSTD_compressBound(last_block_size);
    return bound;
}

struct CompressionStreamEncoder {
    size_t elem_size;
    /* Block size in bytes. */
    uint32_t block_size;
    /* The bit-shuffled block followed by the scratch space of bitshuffle,
     * `block_size` bytes each. */
    char* tmp_buf;
    ZSTD_CCtx* cctx;
    /* Bytes of the current stream that were not compressed yet. */
    uint64_t remaining;
    _Bool started;
};

static size_t stream_encoder_start(CompressionStreamEncoder* encoder,
                                   char* dst,
                                   size_t dst_size,
                                   size_t src_size) {
    encoder->started = 0;
    if (src_size % encoder->elem_size != 0 || dst_size < 12)
        return COMPRESSION_ERROR;
    write_u64_be(dst, src_size);
    write_u32_be(dst + 8, encoder->block_size);
    encoder->remaining = src_size;
    encoder->started = 1;
    return 12;
}

static size_t stream_encoder_block(CompressionStreamEncoder* encoder,
                                   char* dst,
                                   size_t dst_size,
                                   const char* src,
                                   size_t src_size) {
    const uint64_t expected_size = encoder->remaining < encoder->block_size
                                       ? encoder->remaining
                                       : encoder->block_size;
    if (!encoder->started)
        return COMPRESSION_ERROR;
    /* An error leaves the stream incomplete: it must be started again. */
    encoder->started = 0;
    if (src_size == 0 || src_size != expected_size)
        return COMPRESSION_ERROR;

    /* Only the last block can have leftover bytes (block sizes are a
     * multiple of BSHUF_BLOCKED_MULT elements). They are stored as is. */
    const size_t leftover_bytes =
        src_size % (BSHUF_BLOCKED_MULT * encoder->elem_size);
    const size_t block_size = src_size - leftover_bytes;
    size_t n = 0;

    if (block_size > 0) {
        if (dst_size < 4)
            return COMPRESSION_ERROR;
        if (bitshuf_encode_block(&encoder->tmp_buf[0], src,
                                 &encoder->tmp_buf[encoder->block_size],
                                 block_size / encoder->elem_size,
                                 encoder->elem_size) != 0)
        {
            return COMPRESSION_ERROR;
        }
        const size_t compressed_size = ZSTD_compressCCtx(
            encoder->cctx, dst + 4, dst_size - 4, &encoder->tmp_buf[0],
            block_size, BSZSTD_COMPRESSION_LEVEL);
        if (ZSTD_isError(compressed_size) || compressed_size > INT_MAX)
            return COMPRESSION_ERROR;
        write_u32_be(dst, (uint32_t)compressed_size);
        n += 4 + compressed_size;
    }

    if (leftover_bytes > 0) {
        if (dst_size - n < leftover_bytes)
            return COMPRESSION_ERROR;
        memcpy(dst + n, src + block_size, leftover_bytes);
        n += leftover_bytes;
    }

    encoder->remaining -= src_size;
    encoder->started = 1;
    return n;
}

static size_t compress_buffer_bszstd(char* dst,
                                     size_t dst_size,
                                     const char* src,
                                     size_t src_size,
                                     size_t elem_size) {
    CompressionStreamEncoder* encoder =
        compression_stream_encoder_create(COMPRESSION_BSZSTD, 0, elem_size);
    if (!encoder)
        return COMPRESSION_ERROR;

    size_t total = stream_encoder_start(encoder, dst, dst_size, src_size);
    while (total != COMPRESSION_ERROR && encoder->remaining > 0) {
        const size_t block_size = encoder->remaining < encoder->block_size
                                      ? (size_t)encoder->remaining
                                      : encoder->block_size;
        const size_t n = stream_encoder_block(encoder, dst + total,
                                              dst_size - total, src,
                                              block_size);
        total = n != COMPRESSION_ERROR ? total + n : COMPRESSION_ERROR;
        src += block_size;
    }

    compression_stream_encoder_free(encoder);
    return total;
}

size_t compression_compress_bound(CompressionAlgorithm algorithm,
                                  size_t src_size,
                                  size_t elem_size) {
    switch (algorithm) {
        case COMPRESSION_BSZSTD:
            return stream_bound_bszstd(src_size, 0, elem_size);
        case COMPRESSION_TRPX:
            return compression_compress_bound_trpx(src_size, elem_size);
        case COMPRESSION_BSLZ4:
        case COMPRESSION_LZ4:
            break;
    }
    return COMPRESSION_ERROR;
}

size_t compression_compress_buffer(CompressionAlgorithm algorithm,
                                   char* dst,
                                   size_t dst_size,
                                   const char* src,
                                   size_t src_size,
                                   size_t elem_size) {
    switch (algorithm) {
        case COMPRESSION_BSZSTD:
            return compress_buffer_bszstd(dst, dst_size, src, src_size,
                                          elem_size);
        case COMPRESSION_TRPX:
            return compression_compress_buffer_trpx(dst, dst_size, src,
                                                    src_size, elem_size);
        case COMPRESSION_BSLZ4:
        case COMPRESSION_LZ4:
            break;
    }
    return COMPRESSION_ERROR;
}

size_t compression_stream_bound(CompressionAlgorithm algorithm,
                                size_t src_size,
                                size_t block_size,
                                size_t elem_size) {
    switch (algorithm) {
        case COMPRESSION_BSZSTD:
            return stream_bound_bszstd(src_size, block_size, elem_size);
        case COMPRESSION_BSLZ4:
        case COMPRESSION_LZ4:
        case COMPRESSION_TRPX:
            break;
    }
    return COMPRESSION_ERROR;
}

CompressionStreamEncoder* compression_stream_encoder_create(
    CompressionAlgorithm algorithm,
    size_t block_size,
    size_t elem_size) {
    if (algorithm != COMPRESSION_BSZSTD)
        return NULL;
    block_size = bszstd_block_size(block_size, elem_size);
    if (block_size == 0)
        return NULL;

    CompressionStreamEncoder* encoder = calloc(1, sizeof(*encoder));
    if (!encoder)
        return NULL;
    encoder->elem_size = elem_size;
    encoder->block_size = (uint32_t)block_size;
    encoder->tmp_buf = malloc(block_size * 2);
    encoder->cctx = ZSTD_createCCtx();
    if (!encoder->tmp_buf || !encoder->cctx) {
        compression_stream_encoder_free(encoder);
        return NULL;
    }
    return encoder;
}

void compression_stream_encoder_free(CompressionStreamEncoder* encoder) {
    if (!encoder)
        return;
    ZSTD_freeCCtx(encoder->cctx);
    free(encoder->tmp_buf);
    free(encoder);
}

size_t compression_stream_encoder_start(CompressionStreamEncoder* encoder,
                                        char* dst,
                                        size_t dst_size,
                                        size_t src_size) {
    return stream_encoder_start(encoder, dst, dst_size, src_size);
}

size_t compression_stream_encoder_block(CompressionStreamEncoder* encoder,
                                        char* dst,
                                        size_t dst_size,
                                        const char* src,
                                        size_t src_size) {
    return stream_encoder_block(encoder, dst, dst_size, src, src_size);
}
