/*
 * Copyright © 2011,2012,2013  Google, Inc.
 * Copyright © 2021  Khaled Hosny
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

#ifndef HB_MS_FEATURE_RANGES_HH
#define HB_MS_FEATURE_RANGES_HH

#include "hb.hh"

typedef struct hb_ms_feature_t {
  hb_tag_t tag;
  uint32_t value;
} hb_ms_feature_t;

typedef struct hb_ms_features_t {
  hb_ms_feature_t *features;
  uint32_t         num_features;
} hb_ms_features_t;

struct active_feature_t {
  hb_ms_feature_t fea;
  unsigned int order;

  HB_INTERNAL static int cmp (const void *pa, const void *pb) {
    const active_feature_t *a = (const active_feature_t *) pa;
    const active_feature_t *b = (const active_feature_t *) pb;
    return a->fea.tag < b->fea.tag ? -1 : a->fea.tag > b->fea.tag ? 1 :
	   a->order < b->order ? -1 : a->order > b->order ? 1 :
	   a->fea.value < b->fea.value ? -1 : a->fea.value > b->fea.value ? 1 :
	   0;
  }
  bool operator== (const active_feature_t *f)
  { return cmp (this, f) == 0; }
};

struct feature_event_t {
  unsigned int index;
  bool start;
  active_feature_t feature;

  HB_INTERNAL static int cmp (const void *pa, const void *pb)
  {
    const feature_event_t *a = (const feature_event_t *) pa;
    const feature_event_t *b = (const feature_event_t *) pb;
    return a->index < b->index ? -1 : a->index > b->index ? 1 :
	   a->start < b->start ? -1 : a->start > b->start ? 1 :
	   active_feature_t::cmp (&a->feature, &b->feature);
  }
};

struct range_record_t {
  hb_ms_features_t features;
  unsigned int index_first; /* == start */
  unsigned int index_last;  /* == end - 1 */
};

HB_INTERNAL static bool
hb_ms_setup_features (const hb_feature_t          *features,
		      unsigned int                 num_features,
		      hb_vector_t<hb_ms_feature_t> &feature_records, /* OUT */
		      hb_vector_t<range_record_t>  &range_records /* OUT */)
{

  /* Sort features by start/end events. */
  hb_vector_t<feature_event_t> feature_events;
  for (unsigned int i = 0; i < num_features; i++)
  {
    active_feature_t feature;
    feature.fea.tag = hb_uint32_swap (features[i].tag);
    feature.fea.value = features[i].value;
    feature.order = i;

    feature_event_t *event;

    event = feature_events.push ();
    event->index = features[i].start;
    event->start = true;
    event->feature = feature;

    event = feature_events.push ();
    event->index = features[i].end;
    event->start = false;
    event->feature = feature;
  }
  feature_events.qsort ();
  /* Add a strategic final event. */
  {
    active_feature_t feature;
    feature.fea.tag = 0;
    feature.fea.value = 0;
    feature.order = num_features + 1;

    feature_event_t *event = feature_events.push ();
    event->index = 0; /* This value does magic. */
    event->start = false;
    event->feature = feature;
  }

  /* Scan events and save features for each range. */
  hb_vector_t<active_feature_t> active_features;
  unsigned int last_index = 0;
  for (unsigned int i = 0; i < feature_events.length; i++)
  {
    feature_event_t *event = &feature_events[i];

    if (event->index != last_index)
    {
      /* Save a snapshot of active features and the range. */
      range_record_t *range = range_records.push ();

      unsigned int offset = feature_records.length;

      active_features.qsort ();
      for (unsigned int j = 0; j < active_features.length; j++)
      {
        if (!j || active_features[j].fea.tag != feature_records[feature_records.length - 1].tag)
        {
          feature_records.push (active_features[j].fea);
        }
        else
        {
          /* Overrides value for existing feature. */
          feature_records[feature_records.length - 1].value = active_features[j].fea.value;
        }
      }

      /* Will convert to pointer after all is ready, since feature_records.array
       * may move as we grow it. */
      range->features.features = reinterpret_cast<hb_ms_feature_t *> (offset);
      range->features.num_features = feature_records.length - offset;
      range->index_first = last_index;
      range->index_last  = event->index - 1;

      last_index = event->index;
    }

    if (event->start)
    {
      active_features.push (event->feature);
    }
    else
    {
      active_feature_t *feature = active_features.find (&event->feature);
      if (feature)
        active_features.remove (feature - active_features.arrayZ);
    }
  }

  if (!range_records.length) /* No active feature found. */
    num_features = 0;

  /* Fixup the pointers. */
  for (unsigned int i = 0; i < range_records.length; i++)
  {
    range_record_t *range = &range_records[i];
    range->features.features = (hb_ms_feature_t *) feature_records + reinterpret_cast<uintptr_t> (range->features.features);
  }

  return !!num_features;

}

HB_INTERNAL static void
hb_ms_make_feature_ranges (hb_vector_t<hb_ms_feature_t>  &feature_records,
			   hb_vector_t<range_record_t>   &range_records,
			   unsigned int                   chars_offset,
			   unsigned int                   chars_len,
			   uint16_t                      *log_clusters,
			   hb_vector_t<hb_ms_features_t*> &range_features, /* OUT */
			   hb_vector_t<uint32_t>          &range_counts /* OUT */)
{
  range_features.shrink (0);
  range_counts.shrink (0);

  range_record_t *last_range = &range_records[0];
  for (unsigned int i = chars_offset; i < chars_len; i++)
  {
    range_record_t *range = last_range;
    while (log_clusters[i] < range->index_first)
      range--;
    while (log_clusters[i] > range->index_last)
      range++;
    if (!range_features.length ||
        &range->features != range_features[range_features.length - 1])
    {
      auto **features = range_features.push ();
      auto *c = range_counts.push ();
      if (unlikely (!features || !c))
      {
        range_features.shrink (0);
        range_counts.shrink (0);
        break;
      }
      *features = &range->features;
      *c = 1;
    }
    else
    {
      range_counts[range_counts.length - 1]++;
    }

    last_range = range;
  }
}

#endif /* HB_MS_FEATURE_RANGES_HH */
