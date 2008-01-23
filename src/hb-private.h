#ifndef HB_PRIVATE_H
#define HB_PRIVATE_H

#include <assert.h>

#define _ASSERT_STATIC1(_line, _cond) typedef int _static_assert_on_line_##_line##_failed[(_cond)?1:-1]
#define _ASSERT_STATIC0(_line, _cond) _ASSERT_STATIC1 (_line, (_cond))
#define ASSERT_STATIC(_cond) _ASSERT_STATIC0 (__LINE__, (_cond))

#define ASSERT_SIZE(_type, _size) ASSERT_STATIC (sizeof (_type) == (_size))

#endif /* HB_PRIVATE_H */
