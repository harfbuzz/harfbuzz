#include <hb-config.hh>

#include <hb.h>
#include <stddef.h>
#include <string.h>

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size);

#if defined(__GNUC__) && (__GNUC__ >= 4) || (__clang__)
#define HB_UNUSED	__attribute__((unused))
#else
#define HB_UNUSED
#endif

#ifdef HB_IS_IN_FUZZER

/* See src/failing-alloc.c */
extern "C" int alloc_state;

#else

/* Just a dummy global variable */
static int HB_UNUSED alloc_state = 0;

#endif

static inline int
_fuzzing_alloc_state (const uint8_t *data, size_t size)
{
  /* https://github.com/harfbuzz/harfbuzz/pull/2764#issuecomment-1172589849 */

  /* In 50% of the runs, don't fail the allocator. */
  if (size && data[size - 1] < 0x80)
    return 0;

  return size;
}

static inline void
_fuzzing_skip_leading_comment (const uint8_t **data, size_t *size)
{
  if (!*size || **data != '$')
    return;

  const uint8_t *newline = (const uint8_t *) memchr (*data, '\n', *size);
  if (!newline)
    return;

  size_t skipped = (size_t) (newline + 1 - *data);
  *data += skipped;
  *size -= skipped;
}

static const uint8_t _fuzzing_extended_magic[] = {'H', 'B', 'S', 'U', 'B', 'F', 'Z', '2'};

enum _fuzzing_extended_op_t
{
  HB_FUZZING_OP_SET_FLAGS = 1,
  HB_FUZZING_OP_KEEP_EVERYTHING = 2,
  HB_FUZZING_OP_SET_CLEAR = 3,
  HB_FUZZING_OP_SET_INVERT = 4,
  HB_FUZZING_OP_SET_ADD_RANGES = 5,
  HB_FUZZING_OP_SET_DEL_RANGES = 6,
  HB_FUZZING_OP_TEXT_ADD = 7,
  HB_FUZZING_OP_TEXT_DEL = 8,
  HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT = 9,
  HB_FUZZING_OP_AXIS_SET = 10,
};

enum _fuzzing_extended_axis_mode_t
{
  HB_FUZZING_AXIS_PIN_TO_DEFAULT = 0,
  HB_FUZZING_AXIS_SET_RANGE = 1,
};

static inline uint32_t
_fuzzing_read_u32_le (const uint8_t *p)
{
  return (uint32_t) p[0] |
         ((uint32_t) p[1] << 8) |
         ((uint32_t) p[2] << 16) |
         ((uint32_t) p[3] << 24);
}

static inline float
_fuzzing_read_f32_le (const uint8_t *p)
{
  uint32_t bits = _fuzzing_read_u32_le (p);
  float value;
  memcpy (&value, &bits, sizeof (value));
  return value;
}

template <typename T>
static inline bool
_fuzzing_read_value (const uint8_t *&p,
                     const uint8_t *end,
                     T *out)
{
  if ((size_t) (end - p) < sizeof (T))
    return false;

  memcpy (out, p, sizeof (T));
  p += sizeof (T);
  return true;
}

static inline bool
_fuzzing_read_u32_value (const uint8_t *&p,
                         const uint8_t *end,
                         uint32_t *out)
{
  if ((size_t) (end - p) < 4)
    return false;
  *out = _fuzzing_read_u32_le (p);
  p += 4;
  return true;
}

static inline bool
_fuzzing_read_f32_value (const uint8_t *&p,
                         const uint8_t *end,
                         float *out)
{
  if ((size_t) (end - p) < 4)
    return false;
  *out = _fuzzing_read_f32_le (p);
  p += 4;
  return true;
}
