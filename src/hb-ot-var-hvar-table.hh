/*
 * Copyright © 2017  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_VAR_HVAR_TABLE_HH
#define HB_OT_VAR_HVAR_TABLE_HH

#include "hb-ot-layout-common-private.hh"


namespace OT {


struct DeltaSetIndexMap
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_array (this, 1, 12));
  }

  USHORT x;
  public:
  DEFINE_SIZE_STATIC (2);
};


/*
 * HVAR -- The Horizontal Metrics Variations Table
 * VVAR -- The Vertical Metrics Variations Table
 */

#define HB_OT_TAG_HVAR HB_TAG('H','V','A','R')
#define HB_OT_TAG_VVAR HB_TAG('V','V','A','R')

struct HVARVVAR
{
  static const hb_tag_t HVARTag	= HB_OT_TAG_hmtx;
  static const hb_tag_t VVARTag	= HB_OT_TAG_vmtx;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  likely (version.major == 1) &&
		  varStore.sanitize (c, this) &&
		  advMap.sanitize (c, this) &&
		  lsbMap.sanitize (c, this) &&
		  rsbMap.sanitize (c, this));
  }

  protected:
  FixedVersion<>version;	/* Version of the metrics variation table
				 * initially set to 0x00010000u */
  LOffsetTo<VariationStore>
		varStore;	/* Offset to item variation store table. */
  LOffsetTo<DeltaSetIndexMap>
		advMap;		/* Offset to advance var-idx mapping. */
  LOffsetTo<DeltaSetIndexMap>
		lsbMap;		/* Offset to lsb/tsb var-idx mapping. */
  LOffsetTo<DeltaSetIndexMap>
		rsbMap;		/* Offset to rsb/bsb var-idx mapping. */

  public:
  DEFINE_SIZE_STATIC (20);
};

struct HVAR : HVARVVAR {
  static const hb_tag_t tableTag	= HB_OT_TAG_HVAR;
};
struct VVAR : HVARVVAR {
  static const hb_tag_t tableTag	= HB_OT_TAG_VVAR;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (static_cast<const HVARVVAR *> (this)->sanitize (c) &&
		  vorgMap.sanitize (c, this));
  }

  protected:
  LOffsetTo<DeltaSetIndexMap>
		vorgMap;	/* Offset to vertical-origin var-idx mapping. */

  public:
  DEFINE_SIZE_STATIC (24);
};

} /* namespace OT */


#endif /* HB_OT_VAR_HVAR_TABLE_HH */
