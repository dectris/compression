#include <algorithm>
#include <memory>
#include <string>

#include "gtest/gtest.h"

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
}

TEST(Compression, DecompressErrorEmpty) {
    char dst[1], src[1] = {0};
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_BSLZ4, dst, sizeof(dst),
                                            src, 0, 1),
              COMPRESSION_ERROR);
    EXPECT_EQ(compression_decompress_buffer(COMPRESSION_LZ4, dst, sizeof(dst),
                                            src, 0, 0),
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
}

TEST(Compression, DecompressErrorBslz4ZeroElemSize) {
    const auto ENCODED = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s;
    char buffer[1];
    EXPECT_EQ(
        compression_decompress_buffer(COMPRESSION_BSLZ4, buffer, sizeof(buffer),
                                      ENCODED.data(), ENCODED.size(), 0),
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
