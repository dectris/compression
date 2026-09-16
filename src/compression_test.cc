#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "bitshuffle/src/bitshuffle.h"
#include "zstd/lib/zstd.h"

// Concurrent.hpp must be included before Terse.hpp (also for Terse<void>).
#include "Concurrent.hpp"
#include "Terse.hpp"

#include "src/compression.h"

using namespace std::literals;

static bool check_decompress_buffer(CompressionAlgorithm algorithm,
                                    const std::string& encoded,
                                    size_t elem_size,
                                    const std::string& expected) {
    if (compression_decompress_buffer(algorithm, nullptr, 0, encoded.data(),
                                      encoded.size(),
                                      elem_size) != expected.size())
        return false;
    const size_t buffer_size = expected.size() > 0 ? expected.size() : 1;
    auto buffer = std::make_unique<char[]>(buffer_size);
    if (compression_decompress_buffer(algorithm, buffer.get(), buffer_size,
                                      encoded.data(), encoded.size(),
                                      elem_size) != expected.size())
        return false;
    return std::equal(expected.begin(), expected.end(), buffer.get());
}

static size_t decompress_error(CompressionAlgorithm algorithm,
                               const std::string& encoded,
                               size_t elem_size,
                               size_t buffer_size) {
    auto buffer = std::make_unique<char[]>(buffer_size > 0 ? buffer_size : 1);
    return compression_decompress_buffer(algorithm, buffer.get(), buffer_size,
                                         encoded.data(), encoded.size(),
                                         elem_size);
}

template <typename T>
static std::string to_bytes(const std::vector<T>& values) {
    // Little-endian only.
    return std::string(reinterpret_cast<const char*>(values.data()),
                       values.size() * sizeof(T));
}

template <typename T>
static std::vector<T> random_values(std::mt19937_64& rng, size_t n) {
    std::vector<T> values(n);
    for (size_t i = 0; i < n; ++i) {
        // Mostly weak data with some strong and some saturated values.
        const auto r = rng() % 16;
        if (r < 11)
            values[i] = static_cast<T>(rng() % 5);
        else if (r < 15)
            values[i] = static_cast<T>(rng());
        else
            values[i] = std::numeric_limits<T>::max();
    }
    return values;
}

static void append_be32(std::string& out, uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}

static void append_be64(std::string& out, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}

// Encodes `data` in the format of the bitshuffle HDF5 filter with
// BSHUF_H5_COMPRESS_ZSTD. `block_size` is in bytes.
static std::string encode_bszstd(const std::string& data,
                                 size_t elem_size,
                                 size_t block_size) {
    std::string out;
    append_be64(out, data.size());
    append_be32(out, static_cast<uint32_t>(block_size));

    const size_t leftover = data.size() % (8 * elem_size);
    const size_t blocked = data.size() - leftover;
    std::vector<char> shuffled(block_size), scratch(block_size);
    std::vector<char> compressed(ZSTD_compressBound(block_size));
    for (size_t offset = 0; offset < blocked; offset += block_size) {
        const size_t n = std::min(block_size, blocked - offset);
        EXPECT_EQ(bitshuf_encode_block(shuffled.data(), data.data() + offset,
                                       scratch.data(), n / elem_size,
                                       elem_size),
                  0);
        const size_t c = ZSTD_compress(compressed.data(), compressed.size(),
                                       shuffled.data(), n, 1);
        EXPECT_FALSE(ZSTD_isError(c));
        append_be32(out, static_cast<uint32_t>(c));
        out.append(compressed.data(), c);
    }
    out.append(data.data() + blocked, leftover);
    return out;
}

// Encodes `values` as a single TRPX stream with one frame.
template <typename T>
static std::string encode_trpx(const std::vector<T>& values) {
    jpa::Terse<void> terse;
    terse.push_back(values.data(), values.size());
    std::ostringstream os;
    terse.write(os);
    return os.str();
}

TEST(Compression, DecompressBlockUnderflow) {
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSLZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x20\x00\x89"s, 1, "\x89"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSLZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x40\x00\x89\x00"s, 2,
        "\x89\x00"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSLZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x80\x00\x89\x00\x00\x00"s, 4,
        "\x89\x00\x00\x00"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_LZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x20\x00\x00\x00\x00\x01\x89"s,
        0, "\x89"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSZSTD,
        "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x20\x00\x89"s, 1, "\x89"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSZSTD,
        "\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x40\x00\x89\x00"s, 2,
        "\x89\x00"s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSZSTD,
        "\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x80\x00\x89\x00\x00\x00"s, 4,
        "\x89\x00\x00\x00"s));
}

TEST(Compression, DecompressLz4Hdf5CompressedBlocks) {
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_LZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x1d\x00\x00\x00\x10\x00\x00\x00\x0a\x16\xff\x01\x00\x50\xff\xff\xff\xff\xff\x00\x00\x00\x0a\x13\xff\x01\x00\x50\xff\xff\xff\xff\xff"s,
        0,
        "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"s));
}

TEST(Compression, DecompressLz4Hdf5UncompressedBlocks) {
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_LZ4,
        "\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x02\x00\x00\x00\x02\xff\xff\x00\x00\x00\x01\xff"s,
        0, "\xff\xff\xff"s));
}

TEST(Compression, DecompressEmpty) {
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSLZ4, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s,
        1, ""s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_LZ4, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s, 0,
        ""s));
    EXPECT_TRUE(check_decompress_buffer(
        COMPRESSION_BSZSTD, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s,
        1, ""s));
}

TEST(Compression, DecompressErrorEmpty) {
    char dst[1], src[1] = {0};
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSLZ4, dst, sizeof(dst),
                                            src, 0, 1),
              COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_LZ4, dst, sizeof(dst),
                                            src, 0, 0),
              COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSZSTD, dst,
                                            sizeof(dst), src, 0, 1),
              COMPRESSION_ERROR);
}

TEST(Compression, DecompressErrorZeroBlockSize) {
    const auto ENCODED = "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00"s;
    char buffer[1];
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_BSLZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 1),
        COMPRESSION_ERROR);
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_LZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 0),
        COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSZSTD, buffer,
                                            sizeof(buffer), ENCODED.data(),
                                            ENCODED.size(), 1),
              COMPRESSION_ERROR);
}

TEST(Compression, DecompressErrorTruncatedBlockHeader) {
    const auto ENCODED =
        "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00"s;
    char buffer[1];
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_BSLZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 1),
        COMPRESSION_ERROR);
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_LZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 0),
        COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSZSTD, buffer,
                                            sizeof(buffer), ENCODED.data(),
                                            ENCODED.size(), 1),
              COMPRESSION_ERROR);
}

TEST(Compression, DecompressErrorBslz4ZeroElemSize) {
    const auto ENCODED = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s;
    char buffer[1];
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_BSLZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 0),
        COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSZSTD, buffer,
                                            sizeof(buffer), ENCODED.data(),
                                            ENCODED.size(), 0),
              COMPRESSION_ERROR);
}

#if UINT32_MAX <= SIZE_MAX
TEST(Compression, DecompressErrorBslz4ElemSizeOverflow) {
    const auto ENCODED = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s;
    char buffer[1];
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSLZ4, buffer,
                                            sizeof(buffer), ENCODED.data(),
                                            ENCODED.size(), (size_t)0xfffffffe),
              COMPRESSION_ERROR);
}
#endif

TEST(Compression, DecompressErrorLz4BlockSizeOverflow) {
    const auto ENCODED =
        "\x00\x00\x00\x00\xff\xff\xff\xfe\xff\xff\xff\xfe\x00\x00\x00\x01\x00"s;
    char buffer[1];
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_LZ4, buffer, 0xffffffff,
                                            ENCODED.data(), ENCODED.size(), 0),
              COMPRESSION_ERROR);
}

// Streams produced by the DECTRIS champex encoder with a block size of 8
// elements. They cover full blocks, a partial last block and uncompressed
// leftover elements and have been verified to decode through the bitshuffle
// HDF5 filter of hdf5plugin.
static const auto BSZSTD_GOLDEN_U8 =
    "\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x08\x00\x00\x00\x11\x28\xb5\x2f\xfd\x20\x08\x41\x00\x00\x55\xcc\x3c\x56\x98\xe0\x00\x00\x39\x40\x47\x4e"s;
static const std::vector<uint8_t> BSZSTD_GOLDEN_U8_VALUES = {
    1, 8, 15, 22, 29, 36, 43, 50, 57, 64, 71, 78};

static const auto BSZSTD_GOLDEN_U16 =
    "\x00\x00\x00\x00\x00\x00\x00\x26\x00\x00\x00\x10\x00\x00\x00\x19\x28\xb5\x2f\xfd\x20\x10\x81\x00\x00\xff\x00\x55\xcc\x96\xb2\x24\x38\x6a\x4c\x70\x80\x00\x00\x00\x00\x00\x00\x00\x19\x28\xb5\x2f\xfd\x20\x10\x81\x00\x00\xff\x00\x55\xcc\x96\x4d\x69\x8e\xa5\x36\x38\x3f\xc0\x00\x00\x00\xc5\x12\xf1\x13\x1d\x15"s;
static const std::vector<uint16_t> BSZSTD_GOLDEN_U16_VALUES = {
    5,    305,  605,  905,  1205, 1505, 1805, 2105, 2405, 2705,
    3005, 3305, 3605, 3905, 4205, 4505, 4805, 5105, 5405};

static const auto BSZSTD_GOLDEN_U32 =
    "\x00\x00\x00\x00\x00\x00\x00\x60\x00\x00\x00\x20\x00\x00\x00\x23\x28\xb5\x2f\xfd\x20\x20\xd5\x00\x00\xa0\xff\xff\x00\xff\xaa\x66\x1e\x54\x32\xa4\x38\xc0\xaa\xcc\xf0\x00\xaa\xcc\xf0\x00\x01\x00\x38\xc0\x04\x00\x00\x00\x29\x28\xb5\x2f\xfd\x20\x20\x01\x01\x00\xff\xff\x00\xff\xaa\x66\x1e\xab\x99\x2d\xce\x0f\x5a\x6c\x70\x7f\x2a\x4c\x70\x7f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x29\x28\xb5\x2f\xfd\x20\x20\x01\x01\x00\xff\xff\x00\xff\xaa\x66\x1e\x54\xcd\x69\x71\x7e\xd5\x66\x78\x80\x55\x66\x78\x80\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s;
static const std::vector<uint32_t> BSZSTD_GOLDEN_U32_VALUES = {
    11,      70011,   140011,  210011,  280011,  350011,  420011,  490011,
    560011,  630011,  700011,  770011,  840011,  910011,  980011,  1050011,
    1120011, 1190011, 1260011, 1330011, 1400011, 1470011, 1540011, 1610011};

TEST(Compression, DecompressBszstdGolden) {
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_BSZSTD, BSZSTD_GOLDEN_U8,
                                        1, to_bytes(BSZSTD_GOLDEN_U8_VALUES)));
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_BSZSTD, BSZSTD_GOLDEN_U16,
                                        2,
                                        to_bytes(BSZSTD_GOLDEN_U16_VALUES)));
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_BSZSTD, BSZSTD_GOLDEN_U32,
                                        4,
                                        to_bytes(BSZSTD_GOLDEN_U32_VALUES)));
}

template <typename T>
static void check_bszstd_round_trip(std::mt19937_64& rng) {
    for (const size_t n : {1u, 7u, 8u, 9u, 16u, 100u, 1000u, 4099u}) {
        for (const size_t block_elems : {8u, 16u, 512u, 4096u}) {
            const auto data = to_bytes(random_values<T>(rng, n));
            const auto encoded =
                encode_bszstd(data, sizeof(T), block_elems * sizeof(T));
            EXPECT_TRUE(check_decompress_buffer(COMPRESSION_BSZSTD, encoded,
                                                sizeof(T), data))
                << "n=" << n << " block=" << block_elems;
        }
    }
}

TEST(Compression, DecompressBszstdRoundTrip) {
    std::mt19937_64 rng(42);
    check_bszstd_round_trip<uint8_t>(rng);
    check_bszstd_round_trip<uint16_t>(rng);
    check_bszstd_round_trip<uint32_t>(rng);
    check_bszstd_round_trip<uint64_t>(rng);
}

TEST(Compression, DecompressErrorBszstd) {
    const size_t n = to_bytes(BSZSTD_GOLDEN_U16_VALUES).size();

    // Element size does not match the block size.
    EXPECT_EQ(decompress_error(COMPRESSION_BSZSTD, BSZSTD_GOLDEN_U16, 4, n),
              COMPRESSION_ERROR);
    // Destination too small.
    EXPECT_EQ(decompress_error(COMPRESSION_BSZSTD, BSZSTD_GOLDEN_U16, 2, n - 2),
              COMPRESSION_ERROR);
    // Truncated in the middle of a zstd frame.
    EXPECT_EQ(decompress_error(COMPRESSION_BSZSTD,
                               BSZSTD_GOLDEN_U16.substr(0, 30), 2, n),
              COMPRESSION_ERROR);
    // Missing leftover elements.
    EXPECT_EQ(
        decompress_error(COMPRESSION_BSZSTD,
                         BSZSTD_GOLDEN_U16.substr(0, BSZSTD_GOLDEN_U16.size() - 1),
                         2, n),
        COMPRESSION_ERROR);
    // Corrupted zstd frame.
    auto corrupted = BSZSTD_GOLDEN_U16;
    corrupted[20] = static_cast<char>(corrupted[20] ^ 0xff);
    EXPECT_EQ(decompress_error(COMPRESSION_BSZSTD, corrupted, 2, n),
              COMPRESSION_ERROR);
    // An LZ4 stream is not a zstd stream.
    EXPECT_EQ(decompress_error(COMPRESSION_BSZSTD,
                               "\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x08\x00\x00\x00\x09\x80\x00\x01\x02\x03\x04\x05\x06\x07"s,
                               1, 8),
              COMPRESSION_ERROR);
}

// A 4x4 uint16 image [0, 1000, 2000, ..., 15000] with pixel 5 saturated
// (65535), compressed by the DECTRIS champex encoder.
static const auto TRPX_GOLDEN_U16 =
    "<Terse prolix_bits=\"16\" signed=\"0\" block=\"12\" number_of_values=\"16\" number_of_frames=\"1\" memory_sizes_of_frames=\"48 \" memory_size=\"48\" metadata_string_sizes=\"0 \"/>"
    "\xfc\xff\xf3\x9b\x9f\x01\x40\xfa\x10\x7d\xe4\x2e\xa1\x0f\x00\x10\x77\x65\x6d\x41\x5f\xca\x18\x71\xe6\xab\x06\x77\x91\x65\x58\x1b\x53\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s;

static std::vector<uint16_t> trpx_golden_u16_values() {
    std::vector<uint16_t> values(16);
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = static_cast<uint16_t>(i * 1000);
    values[5] = std::numeric_limits<uint16_t>::max();
    return values;
}

TEST(Compression, DecompressTrpxGolden) {
    const auto expected = to_bytes(trpx_golden_u16_values());
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 2,
                                        expected));
    // The element size is optional for TRPX.
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 0,
                                        expected));
}

template <typename T>
static void check_trpx_round_trip(std::mt19937_64& rng) {
    for (const size_t n : {1u, 11u, 12u, 13u, 24u, 77u, 1000u, 4099u}) {
        const auto values = random_values<T>(rng, n);
        const auto encoded = encode_trpx(values);
        ASSERT_EQ(encoded.compare(0, 7, "<Terse "), 0);
        EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX, encoded,
                                            sizeof(T), to_bytes(values)))
            << "n=" << n;
        EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX, encoded, 0,
                                            to_bytes(values)))
            << "n=" << n;
    }
}

TEST(Compression, DecompressTrpxRoundTrip) {
    std::mt19937_64 rng(7);
    check_trpx_round_trip<uint8_t>(rng);
    check_trpx_round_trip<uint16_t>(rng);
    check_trpx_round_trip<uint32_t>(rng);
}

TEST(Compression, DecompressTrpxMultipleFrames) {
    std::mt19937_64 rng(11);
    const auto a = random_values<uint16_t>(rng, 100);
    const auto b = random_values<uint16_t>(rng, 100);
    jpa::Terse<void> terse;
    terse.push_back(a.data(), a.size());
    terse.push_back(b.data(), b.size());
    std::ostringstream os;
    terse.write(os);
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX, os.str(), 2,
                                        to_bytes(a) + to_bytes(b)));
}

TEST(Compression, DecompressTrpxConcatenatedStreams) {
    // The TERSE HDF5 filter writes chunks as concatenated TRPX streams.
    std::mt19937_64 rng(13);
    const auto a = random_values<uint32_t>(rng, 1000);
    const auto b = random_values<uint32_t>(rng, 17);
    EXPECT_TRUE(check_decompress_buffer(COMPRESSION_TRPX,
                                        encode_trpx(a) + encode_trpx(b), 4,
                                        to_bytes(a) + to_bytes(b)));
}

TEST(Compression, DecompressErrorTrpx) {
    const size_t n = to_bytes(trpx_golden_u16_values()).size();

    // Wrong or unsupported element size.
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 1, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 4, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 3, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 8, n),
              COMPRESSION_ERROR);
    // Destination too small.
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16, 2, n - 2),
              COMPRESSION_ERROR);
    // Empty input and garbage.
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, ""s, 2, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, ""s, 2, 0),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, "this is not a trpx stream"s,
                               2, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, "this is not a trpx stream"s,
                               0, 0),
              COMPRESSION_ERROR);
    // Truncated header and truncated frame data.
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16.substr(0, 40),
                               2, n),
              COMPRESSION_ERROR);
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX,
                               TRPX_GOLDEN_U16.substr(0, TRPX_GOLDEN_U16.size() - 8),
                               2, n),
              COMPRESSION_ERROR);
    // Trailing garbage after a valid stream.
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, TRPX_GOLDEN_U16 + "garbage"s,
                               2, n),
              COMPRESSION_ERROR);
    // Signed data is not supported.
    const std::vector<int16_t> signed_values = {-1, 0, 1, 2, 3, 4, 5, 6};
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX, encode_trpx(signed_values), 2,
                               signed_values.size() * 2),
              COMPRESSION_ERROR);
    // Concatenated streams with different element sizes.
    const std::vector<uint8_t> u8_values = {1, 2, 3};
    EXPECT_EQ(decompress_error(COMPRESSION_TRPX,
                               TRPX_GOLDEN_U16 + encode_trpx(u8_values), 0,
                               n + u8_values.size()),
              COMPRESSION_ERROR);
}
