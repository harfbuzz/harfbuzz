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
#define HB_WASM_API(x) x
#endif
#ifndef HB_WASM_EXEC_ENV
#define HB_WASM_EXEC_ENV
#endif


#ifndef bool_t
#define bool_t uint32_t
#endif
typedef uint32_t face_t;
typedef uint32_t font_t;
typedef uint32_t buffer_t;


/* font */

face_t
HB_WASM_API (font_get_face) (HB_WASM_EXEC_ENV
			     font_t);


/* shape interface */

bool_t
HB_WASM_API (shape) (HB_WASM_EXEC_ENV
		     font_t,
		     buffer_t);


HB_WASM_END_DECLS

#endif /* HB_WASM_API_H */
