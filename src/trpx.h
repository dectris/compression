#ifndef DECTRIS_COMPRESSION_TRPX_H_
#define DECTRIS_COMPRESSION_TRPX_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Implementation of COMPRESSION_TRPX, see compression_decompress_buffer(). */
size_t compression_decompress_buffer_trpx(char* dst,
                                          size_t dst_size,
                                          const char* src,
                                          size_t src_size,
                                          size_t elem_size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* DECTRIS_COMPRESSION_TRPX_H_ */
