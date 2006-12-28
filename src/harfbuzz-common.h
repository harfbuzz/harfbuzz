#ifndef HARFBUZZ_COMMON_H
#define HARFBUZZ_COMMON_H

#include <stdint.h>

# ifdef __cplusplus
#  define HARFBUZZ_BEGIN_DECLS() extern "C" { extern int harfbuzz_dummy_prototype (int)
#  define HARFBUZZ_END_DECLS() } extern "C" int harfbuzz_dummy_prototype (int)
# else /* !__cplusplus */
#  define HARFBUZZ_BEGIN_DECLS()   extern int harfbuzz_dummy_prototype (int)
#  define HARFBUZZ_END_DECLS()     extern int harfbuzz_dummy_prototype (int)
# endif /* !__cplusplus */

#endif /* HARFBUZZ_COMMON_H */
