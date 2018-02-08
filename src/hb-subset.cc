/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Garret Rieger, Rod Sheeter
 */

#include "hb-object-private.hh"
#include "hb-private.hh"

#include "hb-subset-glyf.hh"
#include "hb-subset-private.hh"
#include "hb-subset-plan.hh"

#include "hb-open-file-private.hh"
#include "hb-ot-glyf-table.hh"


#ifndef HB_NO_VISIBILITY
const void * const OT::_hb_NullPool[HB_NULL_POOL_SIZE / sizeof (void *)] = {};
#endif


struct hb_subset_profile_t {
  hb_object_header_t header;
  ASSERT_POD ();
};

/**
 * hb_subset_profile_create:
 *
 * Return value: New profile with default settings.
 *
 * Since: 1.7.5
 **/
hb_subset_profile_t *
hb_subset_profile_create ()
{
  return hb_object_create<hb_subset_profile_t>();
}

/**
 * hb_subset_profile_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_profile_destroy (hb_subset_profile_t *profile)
{
  if (!hb_object_destroy (profile)) return;

  free (profile);
}

/**
 * hb_subset_input_create:
 *
 * Return value: New subset input.
 *
 * Since: 1.7.5
 **/
hb_subset_input_t *
hb_subset_input_create (hb_set_t *codepoints)
{
  if (unlikely (!codepoints))
    codepoints = hb_set_get_empty();

  hb_subset_input_t *input = hb_object_create<hb_subset_input_t>();
  input->codepoints = hb_set_reference(codepoints);
  return input;
}

/**
 * hb_subset_input_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_input_destroy(hb_subset_input_t *subset_input)
{
  if (!hb_object_destroy (subset_input)) return;

  hb_set_destroy (subset_input->codepoints);
  free (subset_input);
}



/*
 * A face that has add_table().
 */

struct hb_subset_face_data_t
{
  struct table_entry_t
  {
    inline int cmp (const hb_tag_t *t) const
    {
      if (*t < tag) return -1;
      if (*t > tag) return -1;
      return 0;
    }

    hb_tag_t   tag;
    hb_blob_t *blob;
  };

  hb_prealloced_array_t<table_entry_t, 32> tables;
};

static hb_subset_face_data_t *
_hb_subset_face_data_create (void)
{
  hb_subset_face_data_t *data = (hb_subset_face_data_t *) calloc (1, sizeof (hb_subset_face_data_t));
  if (unlikely (!data))
    return nullptr;

  return data;
}

static void
_hb_subset_face_data_destroy (void *user_data)
{
  hb_subset_face_data_t *data = (hb_subset_face_data_t *) user_data;

  data->tables.finish ();

  free (data);
}

static hb_blob_t *
_hb_subset_face_data_reference_blob (hb_subset_face_data_t *data)
{

  unsigned int table_count = data->tables.len;
  unsigned int face_length = table_count * 16 + 12;

  for (unsigned int i = 0; i < table_count; i++)
    face_length += _hb_ceil_to_4 (hb_blob_get_length (data->tables.array[i].blob));

  char *buf = (char *) malloc (face_length);
  if (unlikely (!buf))
    return nullptr;

  OT::hb_serialize_context_t c (buf, face_length);
  OT::OpenTypeFontFile *f = c.start_serialize<OT::OpenTypeFontFile> ();

  bool is_cff = data->tables.lsearch (HB_TAG ('C','F','F',' ')) || data->tables.lsearch (HB_TAG ('C','F','F','2'));
  hb_tag_t sfnt_tag = is_cff ? OT::OpenTypeFontFile::CFFTag : OT::OpenTypeFontFile::TrueTypeTag;

  OT::Supplier<hb_tag_t>    tags_supplier  (&data->tables[0].tag, table_count, sizeof (data->tables[0]));
  OT::Supplier<hb_blob_t *> blobs_supplier (&data->tables[0].blob, table_count, sizeof (data->tables[0]));
  bool ret = f->serialize_single (&c,
				  sfnt_tag,
				  tags_supplier,
				  blobs_supplier,
				  table_count);

  c.end_serialize ();

  if (unlikely (!ret))
  {
    free (buf);
    return nullptr;
  }

  return hb_blob_create (buf, face_length, HB_MEMORY_MODE_WRITABLE, buf, free);
}

static hb_blob_t *
_hb_subset_face_reference_table (hb_face_t *face, hb_tag_t tag, void *user_data)
{
  hb_subset_face_data_t *data = (hb_subset_face_data_t *) user_data;

  if (!tag)
    return _hb_subset_face_data_reference_blob (data);

  hb_subset_face_data_t::table_entry_t *entry = data->tables.lsearch (tag);
  if (entry)
    return hb_blob_reference (entry->blob);

  return nullptr;
}

static hb_face_t *
hb_subset_face_create (void)
{
  hb_subset_face_data_t *data = _hb_subset_face_data_create ();
  if (unlikely (!data)) return hb_face_get_empty ();

  return hb_face_create_for_tables (_hb_subset_face_reference_table,
				    data,
				    _hb_subset_face_data_destroy);
}

static bool
hb_subset_face_add_table (hb_face_t *face, hb_tag_t tag, hb_blob_t *blob)
{
  if (unlikely (face->destroy != _hb_subset_face_data_destroy))
    return false;

  hb_subset_face_data_t *data = (hb_subset_face_data_t *) face->user_data;

  hb_subset_face_data_t::table_entry_t *entry = data->tables.push ();
  if (unlikely (!entry))
    return false;

  entry->tag = tag;
  entry->blob = hb_blob_reference (blob);

  return true;
}

/**
 * hb_subset:
 * @source: font face data to be subset.
 * @profile: profile to use for the subsetting.
 * @input: input to use for the subsetting.
 *
 * Subsets a font according to provided profile and input.
 **/
hb_face_t *
hb_subset (hb_face_t *source,
	   hb_subset_profile_t *profile,
           hb_subset_input_t *input)
{
  if (unlikely (!profile || !input || !source)) return nullptr;

  hb_subset_plan_t *plan = hb_subset_plan_create (source, profile, input);

  hb_face_t *face = hb_subset_face_create ();

  /* Copy tables to new face. */
  {
    hb_tag_t table_tags[32];
    unsigned int offset = 0, count;
    do {
      count = ARRAY_LENGTH (table_tags);
      hb_face_get_table_tags (source, offset, &count, table_tags);
      for (unsigned int i = 0; i < count; i++)
      {
	hb_tag_t tag = table_tags[i];
	hb_blob_t *blob = hb_face_reference_table (source, tag);
	hb_subset_face_add_table (face, tag, blob);
	hb_blob_destroy (blob);
      }
    } while (count == ARRAY_LENGTH (table_tags));
  }

  hb_codepoint_t old_gid = -1;
  while (hb_set_next (plan->glyphs_to_retain, &old_gid)) {
    hb_codepoint_t new_gid;
    if (hb_subset_plan_new_gid_for_old_id (plan, old_gid, &new_gid)) {
      DEBUG_MSG (SUBSET, nullptr, "Remap %d : %d", old_gid, new_gid);
    } else {
      DEBUG_MSG (SUBSET, nullptr, "Remap %d : DOOM! No new ID", old_gid);
    }
  }
  // TODO:
  // - Create initial header + table directory
  // - Loop through the set of tables to be kept:
  //   - Perform table specific subsetting if defined.
  //   - copy the table into the output.
  // - Fix header + table directory.

  hb_blob_t *glyf_prime = nullptr;
  if (hb_subset_glyf (plan, source, &glyf_prime)) {
    // TODO: write new glyf to new face.
  }
  hb_blob_destroy (glyf_prime);

  hb_subset_plan_destroy (plan);

  return face;
}
