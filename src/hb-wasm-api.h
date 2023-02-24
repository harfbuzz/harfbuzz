/*
 * Copyright © 2023  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef HB_WASM_API_H
#define HB_WASM_API_H

/*
#include "hb.h"
*/

#include <stdint.h>


#ifndef HB_WASM_BEGIN_DECLS
# ifdef __cplusplus
#  define HB_WASM_BEGIN_DECLS	extern "C" {
#  define HB_WASM_END_DECLS	}
# else /* !__cplusplus */
#  define HB_WASM_BEGIN_DECLS
#  define HB_WASM_END_DECLS
# endif /* !__cplusplus */
#endif


HB_WASM_BEGIN_DECLS

#ifndef HB_WASM_API
#define HB_WASM_API(ret_t, name) ret_t name
#endif
#ifndef HB_WASM_API_COMPOUND /* compound return type */
#define HB_WASM_API_COMPOUND(ret_t, name) HB_WASM_API(ret_t, name)
#endif
#ifndef HB_WASM_INTERFACE
#define HB_WASM_INTERFACE(ret_t, name) ret_t name
#endif
#ifndef HB_WASM_EXEC_ENV
#define HB_WASM_EXEC_ENV
#endif
#ifndef HB_WASM_EXEC_ENV_COMPOUND
#define HB_WASM_EXEC_ENV_COMPOUND HB_WASM_EXEC_ENV
#endif


#ifndef bool_t
#define bool_t uint32_t
#endif
#ifndef ptr_t
#define ptr_t(type_t) type_t *
#endif

typedef uint32_t codepoint_t;
typedef int32_t position_t;
typedef uint32_t mask_t;
typedef uint32_t tag_t;
#define TAG(c1,c2,c3,c4) ((tag_t)((((uint32_t)(c1)&0xFF)<<24)|(((uint32_t)(c2)&0xFF)<<16)|(((uint32_t)(c3)&0xFF)<<8)|((uint32_t)(c4)&0xFF)))

typedef enum {
  DIRECTION_INVALID = 0,
  DIRECTION_LTR = 4,
  DIRECTION_RTL,
  DIRECTION_TTB,
  DIRECTION_BTT
} direction_t;
#define DIRECTION_IS_VALID(dir)		((((unsigned int) (dir)) & ~3U) == 4)
#define DIRECTION_IS_HORIZONTAL(dir)	((((unsigned int) (dir)) & ~1U) == 4)
#define DIRECTION_IS_VERTICAL(dir)	((((unsigned int) (dir)) & ~1U) == 6)
#define DIRECTION_IS_FORWARD(dir)	((((unsigned int) (dir)) & ~2U) == 4)
#define DIRECTION_IS_BACKWARD(dir)	((((unsigned int) (dir)) & ~2U) == 5)
#define DIRECTION_REVERSE(dir)		((direction_t) (((unsigned int) (dir)) ^ 1))


/* blob */

typedef struct
{
  uint32_t length;
  ptr_t(char) data;
} blob_t;

HB_WASM_API (void, blob_free) (HB_WASM_EXEC_ENV
			       ptr_t(blob_t));


/* buffer */

typedef struct
{
  uint32_t codepoint;
  uint32_t mask;
  uint32_t cluster;
  uint32_t var1;
  uint32_t var2;
} glyph_info_t;

typedef struct
{
  position_t x_advance;
  position_t y_advance;
  position_t x_offset;
  position_t y_offset;
  uint32_t var;
} glyph_position_t;

typedef struct
{
  uint32_t length;
  ptr_t(glyph_info_t) info;
  ptr_t(glyph_position_t) pos;
} buffer_contents_t;

HB_WASM_API (void, buffer_contents_realloc) (HB_WASM_EXEC_ENV
					     ptr_t(buffer_contents_t),
					     uint32_t size);

HB_WASM_API (void, buffer_contents_free) (HB_WASM_EXEC_ENV
					  ptr_t(buffer_contents_t));

typedef struct buffer_t buffer_t;

HB_WASM_API_COMPOUND (buffer_contents_t, buffer_copy_contents) (HB_WASM_EXEC_ENV_COMPOUND
								ptr_t(buffer_t));

HB_WASM_API (bool_t, buffer_set_contents) (HB_WASM_EXEC_ENV
					   ptr_t(buffer_t),
					   ptr_t(const buffer_contents_t));

HB_WASM_API (direction_t, buffer_get_direction) (HB_WASM_EXEC_ENV
						 ptr_t(buffer_t));

HB_WASM_API (void, buffer_reverse) (HB_WASM_EXEC_ENV
				    ptr_t(buffer_t));

HB_WASM_API (void, buffer_reverse_clusters) (HB_WASM_EXEC_ENV
					     ptr_t(buffer_t));

/* face */

typedef struct face_t face_t;

HB_WASM_API_COMPOUND (blob_t, face_reference_table) (HB_WASM_EXEC_ENV_COMPOUND
						     ptr_t(face_t),
						     tag_t table_tag);

HB_WASM_API (unsigned, face_get_upem) (HB_WASM_EXEC_ENV
				       ptr_t(face_t));


/* font */

typedef struct font_t font_t;

HB_WASM_API (ptr_t(face_t), font_get_face) (HB_WASM_EXEC_ENV
					    ptr_t(font_t));

HB_WASM_API (void, font_get_scale) (HB_WASM_EXEC_ENV
				    ptr_t(font_t),
				    ptr_t(int32_t) x_scale,
				    ptr_t(int32_t) y_scale);

HB_WASM_API (codepoint_t, font_get_glyph) (HB_WASM_EXEC_ENV
					      ptr_t(font_t),
					      codepoint_t unicode,
					      codepoint_t variation_selector);

HB_WASM_API (position_t, font_get_glyph_h_advance) (HB_WASM_EXEC_ENV
						    ptr_t(font_t),
						    codepoint_t glyph);

HB_WASM_API (position_t, font_get_glyph_v_advance) (HB_WASM_EXEC_ENV
						    ptr_t(font_t),
						    codepoint_t glyph);

typedef struct {
  position_t x_bearing;
  position_t y_bearing;
  position_t width;
  position_t height;
} glyph_extents_t;

HB_WASM_API (bool_t, font_get_glyph_extents) (HB_WASM_EXEC_ENV
					      ptr_t(font_t),
					      codepoint_t glyph,
					      ptr_t(glyph_extents_t) extents);

HB_WASM_API (void, font_glyph_to_string) (HB_WASM_EXEC_ENV
					  ptr_t(font_t),
					  codepoint_t glyph,
					  char *s, uint32_t size);


/* shape */

HB_WASM_INTERFACE (bool_t, shape_with) (HB_WASM_EXEC_ENV
				        ptr_t(font_t),
				        ptr_t(buffer_t),
					const char *shaper);


/* shape interface */

HB_WASM_INTERFACE (bool_t, shape) (HB_WASM_EXEC_ENV
				   ptr_t(font_t),
				   ptr_t(buffer_t));


HB_WASM_END_DECLS

#endif /* HB_WASM_API_H */
