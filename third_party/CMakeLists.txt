add_library(bitshuffle_core STATIC
  bitshuffle/src/bitshuffle_core.c
  bitshuffle/src/bitshuffle_core.h
  bitshuffle/src/bitshuffle_internals.h
  bitshuffle/src/iochain.c
  bitshuffle/src/iochain.h
  )

add_library(lz4 STATIC
  lz4/lib/lz4.c
  lz4/lib/lz4.h
  )
