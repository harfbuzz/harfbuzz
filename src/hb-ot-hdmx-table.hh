/*
 * Copyright © 2018  Google, Inc.
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
 *
 * Google Author(s): Garret Rieger
 */

#ifndef HB_OT_HDMX_TABLE_HH
#define HB_OT_HDMX_TABLE_HH

#include "hb-open-type-private.hh"

namespace OT {

#define HB_OT_TAG_hdmx HB_TAG('h','d','m','x')


struct DeviceRecord
{
  HBUINT8 pixel_size;
  HBUINT8 max_width;
  HBUINT8 widths[VAR];
};

struct hdmx
{
  inline unsigned int get_size (void) const
  {
    return min_size + num_records * size_device_record;
  }

  inline const DeviceRecord& operator [] (unsigned int i) const
  {
    if (unlikely (i >= num_records)) return Null(DeviceRecord);
    return StructAtOffset<DeviceRecord> (this, min_size + i * size_device_record);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this)
                          && version == 0));
  }

  HBUINT16 version;
  HBINT16  num_records;
  HBINT32  size_device_record;

  DeviceRecord records[VAR];

  DEFINE_SIZE_MIN (8);

};

} /* namespace OT */


#endif /* HB_OT_HDMX_TABLE_HH */
