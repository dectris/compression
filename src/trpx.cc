#include "src/trpx.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <ios>
#include <istream>
#include <streambuf>

// Concurrent.hpp must be included before Terse.hpp (also for Terse<void>).
#include "Concurrent.hpp"
#include "Terse.hpp"

#include "src/compression.h"

namespace {

// Input stream buffer reading from a fixed size memory region. Supports
// seeking, which is required by the XML header parser of Terse.
class MemoryIStreamBuf final : public std::streambuf {
public:
    MemoryIStreamBuf(const char* data, size_t size)
        : begin_(const_cast<char*>(data)), end_(begin_ + size) {
        setg(begin_, begin_, end_);
    }

    size_t remaining() const { return static_cast<size_t>(egptr() - gptr()); }

protected:
    pos_type seekoff(off_type off,
                     std::ios_base::seekdir dir,
                     std::ios_base::openmode) override {
        // Computed as offsets (std::streamoff is 64 bit also on 32 bit
        // platforms) to check the bounds before forming the pointer.
        const off_type size = end_ - begin_;
        off_type base = 0;
        switch (dir) {
            case std::ios_base::beg:
                base = 0;
                break;
            case std::ios_base::end:
                base = size;
                break;
            default:
                base = gptr() - begin_;
                break;
        }
        if (off < -base || off > size - base)
            return pos_type(off_type(-1));
        const off_type pos = base + off;
        setg(begin_, begin_ + static_cast<std::ptrdiff_t>(pos), end_);
        return pos_type(pos);
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override {
        return seekoff(off_type(pos), std::ios_base::beg, mode);
    }

private:
    char* const begin_;
    char* const end_;
};

template <typename T>
void prolix_frames(jpa::Terse<void>& terse, char* dst) {
    T* out = reinterpret_cast<T*>(dst);
    for (size_t frame = 0; frame < terse.number_of_frames(); ++frame) {
        terse.prolix(out, frame);
        out += terse.size();
    }
}

}  // namespace

extern "C" size_t compression_decompress_buffer_trpx(char* dst,
                                                     size_t dst_size,
                                                     const char* src,
                                                     size_t src_size,
                                                     size_t elem_size) {
    if (elem_size != 0 && elem_size != 1 && elem_size != 2 && elem_size != 4)
        return COMPRESSION_ERROR;
    // At least one TRPX header is required.
    if (src_size == 0)
        return COMPRESSION_ERROR;

    try {
        MemoryIStreamBuf buf(src, src_size);
        std::istream is(&buf);
        size_t total = 0;
        while (buf.remaining() > 0) {
            const size_t remaining_before = buf.remaining();
            jpa::Terse<void> terse(is);
            // Truncated stream, or no TRPX header found before the end.
            if (is.fail() || is.eof())
                return COMPRESSION_ERROR;
            // Guard against an object that did not consume any input.
            if (buf.remaining() == remaining_before)
                return COMPRESSION_ERROR;
            if (terse.is_signed())
                return COMPRESSION_ERROR;
            const unsigned bits = terse.bits_per_val();
            if (bits != 8 && bits != 16 && bits != 32)
                return COMPRESSION_ERROR;
            const size_t stream_elem_size = bits / 8;
            if (elem_size == 0)
                elem_size = stream_elem_size;
            else if (elem_size != stream_elem_size)
                return COMPRESSION_ERROR;

            const size_t frames = terse.number_of_frames();
            const size_t values = terse.size();
            if (values != 0 && frames > SIZE_MAX / values)
                return COMPRESSION_ERROR;
            const size_t count = frames * values;
            if (count > SIZE_MAX / elem_size)
                return COMPRESSION_ERROR;
            const size_t bytes = count * elem_size;
            if (bytes > SIZE_MAX - total)
                return COMPRESSION_ERROR;

            if (dst_size != 0) {
                if (bytes > dst_size - total)
                    return COMPRESSION_ERROR;
                switch (elem_size) {
                    case 1:
                        prolix_frames<uint8_t>(terse, dst + total);
                        break;
                    case 2:
                        prolix_frames<uint16_t>(terse, dst + total);
                        break;
                    case 4:
                        prolix_frames<uint32_t>(terse, dst + total);
                        break;
                }
            }
            total += bytes;
        }
        return total;
    } catch (const std::exception&) {
        return COMPRESSION_ERROR;
    }
}
