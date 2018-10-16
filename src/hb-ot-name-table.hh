/*
 * Copyright © 2011,2012  Google, Inc.
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

#ifndef HB_OT_NAME_TABLE_HH
#define HB_OT_NAME_TABLE_HH

#include "hb-open-type.hh"


namespace OT {


/*
 * name -- Naming
 * https://docs.microsoft.com/en-us/typography/opentype/spec/name
 */
#define HB_OT_TAG_name HB_TAG('n','a','m','e')

#define UNSUPPORTED	42

struct NameRecord
{
  static int cmp (const void *pa, const void *pb)
  {
    const NameRecord *a = (const NameRecord *) pa;
    const NameRecord *b = (const NameRecord *) pb;
    int ret;
    ret = b->platformID.cmp (a->platformID);
    if (ret) return ret;
    ret = b->encodingID.cmp (a->encodingID);
    if (ret) return ret;
    ret = b->languageID.cmp (a->languageID);
    if (ret) return ret;
    ret = b->nameID.cmp (a->nameID);
    if (ret) return ret;
    return 0;
  }

  inline uint16_t score (void) const
  {
    /* Same order as in cmap::find_best_subtable(). */
    unsigned int p = platformID;
    unsigned int e = encodingID;

    /* 32-bit. */
    if (p == 3 && e == 10) return 0;
    if (p == 0 && e ==  6) return 1;
    if (p == 0 && e ==  4) return 2;

    /* 16-bit. */
    if (p == 3 && e ==  1) return 3;
    if (p == 0 && e ==  3) return 4;
    if (p == 0 && e ==  2) return 5;
    if (p == 0 && e ==  1) return 6;
    if (p == 0 && e ==  0) return 7;

    /* Symbol. */
    if (p == 3 && e ==  0) return 8;

    /* TODO Apple MacRoman (0:1). */

    return UNSUPPORTED;
  }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    /* We can check from base all the way up to the end of string... */
    return_trace (c->check_struct (this) && c->check_range ((char *) base, (unsigned int) length + offset));
  }

  HBUINT16	platformID;	/* Platform ID. */
  HBUINT16	encodingID;	/* Platform-specific encoding ID. */
  HBUINT16	languageID;	/* Language ID. */
  HBUINT16	nameID;		/* Name ID. */
  HBUINT16	length;		/* String length (in bytes). */
  HBUINT16	offset;		/* String offset from start of storage area (in bytes). */
  public:
  DEFINE_SIZE_STATIC (12);
};

static int
_hb_ot_name_entry_cmp (const void *pa, const void *pb)
{
  const hb_ot_name_entry_t *a = (const hb_ot_name_entry_t *) pa;
  const hb_ot_name_entry_t *b = (const hb_ot_name_entry_t *) pb;

  /* Sort by name_id, then language, then score, then index. */

  if (a->name_id != b->name_id)
    return a->name_id < b->name_id ? -1 : +1;

  int e = strcmp (hb_language_to_string (a->language),
		  hb_language_to_string (b->language));
  if (e)
    return e;

  if (a->score != b->score)
    return a->score < b->score ? -1 : +1;

  if (a->index != b->index)
    return a->index < b->index ? -1 : +1;

  return 0;
}

struct name
{
  static const hb_tag_t tableTag	= HB_OT_TAG_name;

  inline hb_bytes_t get_name (unsigned int idx) const
  {
    const hb_array_t<NameRecord> &all_names = hb_array_t<NameRecord> (nameRecordZ.arrayZ, count);
    const NameRecord &record = all_names[idx];
    return hb_bytes_t ((char *) this + stringOffset + record.offset, record.length);
  }

  inline unsigned int get_size (void) const
  { return min_size + count * nameRecordZ[0].min_size; }

  inline bool sanitize_records (hb_sanitize_context_t *c) const {
    TRACE_SANITIZE (this);
    char *string_pool = (char *) this + stringOffset;
    unsigned int _count = count;
    /* Move to run-time?! */
    for (unsigned int i = 0; i < _count; i++)
      if (!nameRecordZ[i].sanitize (c, string_pool)) return_trace (false);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  likely (format == 0 || format == 1) &&
		  c->check_array (nameRecordZ.arrayZ, count) &&
		  sanitize_records (c));
  }

  struct accelerator_t
  {
    inline void init (hb_face_t *face)
    {
      this->blob = hb_sanitize_context_t().reference_table<name> (face);
      this->table = this->blob->as<name> ();
      const hb_array_t<NameRecord> &all_names = hb_array_t<NameRecord> (this->table->nameRecordZ.arrayZ,
									this->table->count);

      this->names.init ();

      for (uint16_t i = 0; i < all_names.len; i++)
      {
	unsigned int name_id = all_names[i].nameID;
	uint16_t score = all_names[i].score ();
	hb_language_t language = HB_LANGUAGE_INVALID; /* XXX */

	hb_ot_name_entry_t entry = {name_id, score, i, language};

	this->names.push (entry);
      }

      this->names.qsort (_hb_ot_name_entry_cmp);
      /* Walk and pick best only for each name_id,language pair... */
      unsigned int j = 0;
      for (unsigned int i = 1; i < this->names.len; i++)
      {
        if (this->names[i - 1].name_id  == this->names[i].name_id &&
	    this->names[i - 1].language == this->names[i].language)
	  continue;
	this->names[j++] = this->names[i];
      }
      this->names.resize (j);
    }


    inline void fini (void)
    {
      this->names.fini ();
      hb_blob_destroy (this->blob);
    }

    private:
    hb_blob_t *blob;
    public:
    const name *table;
    hb_vector_t<hb_ot_name_entry_t> names;
  };

  /* We only implement format 0 for now. */
  HBUINT16	format;			/* Format selector (=0/1). */
  HBUINT16	count;			/* Number of name records. */
  Offset16	stringOffset;		/* Offset to start of string storage (from start of table). */
  UnsizedArrayOf<NameRecord>
		nameRecordZ;		/* The name records where count is the number of records. */
  public:
  DEFINE_SIZE_ARRAY (6, nameRecordZ);
};

struct name_accelerator_t : name::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_NAME_TABLE_HH */
