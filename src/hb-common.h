#ifndef HB_COMMON_H
#define HB_COMMON_H

#include <stdint.h>

# ifdef __cplusplus
#  define HB_BEGIN_DECLS() extern "C" { extern int hb_dummy_prototype (int)
#  define HB_END_DECLS() } extern "C" int hb_dummy_prototype (int)
# else /* !__cplusplus */
#  define HB_BEGIN_DECLS()   extern int hb_dummy_prototype (int)
#  define HB_END_DECLS()     extern int hb_dummy_prototype (int)
# endif /* !__cplusplus */

#endif /* HB_COMMON_H */
