#ifndef HB_OT_LAYOUT_OPEN_H
#define HB_OT_LAYOUT_OPEN_H

#include "hb-common.h"

HB_BEGIN_DECLS();

typedef uint32_t hb_tag_t;
#define HB_TAG(a,b,c,d) ((hb_tag_t)(((uint8_t)a<<24)|((uint8_t)b<<16)|((uint8_t)c<<8)|(uint8_t)d))
#define HB_TAG_STR(s)   (HB_TAG(((const char *) s)[0], \
				((const char *) s)[1], \
				((const char *) s)[2], \
				((const char *) s)[3]))

HB_END_DECLS();

#endif /* HB_OT_LAYOUT_OPEN_H */
