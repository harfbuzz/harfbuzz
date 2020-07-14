#include <hb.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

#ifdef HB_IS_IN_FUZZER
/* See src/failing-alloc.c */
extern "C" int alloc_state;
#else
/* Just a dummy global variable */
static int alloc_state = 0;
#endif
