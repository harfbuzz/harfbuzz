/*
 * Copyright © 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-vector-paint.hh"
#include "hb-vector-draw.hh"
#include "hb-paint.hh"
#include "hb-vector-paint-pdf-image.hh"

#include <math.h>
#include <stdio.h>


/* PDF paint backend for COLRv0/v1 color font rendering.
 *
 * Supports:
 *   - Solid colors with alpha (via ExtGState)
 *   - Glyph and rectangle clipping
 *   - Affine transforms
 *   - Groups (save/restore)
 *   - Linear gradients (PDF Type 2 axial shading, PAD extension only)
 *   - Radial gradients (PDF Type 3 radial shading, PAD extension only)
 *   - Sweep gradients (PDF Type 6 Coons-patch meshes)
 */


/* ---- PDF object collector ---- */

struct hb_pdf_obj_t
{
  hb_vector_buf_t data;
};

/* Collects extra PDF objects (shadings, functions, ExtGState)
 * during painting.  Referenced from the content stream by name
 * (e.g. /SH0, /GS0) and emitted at render time. */
struct hb_pdf_resources_t
{
  hb_vector_t<hb_pdf_obj_t> objects;   /* extra objects, starting at id 5 */
  hb_vector_buf_t extgstate_dict;    /* /GS0 5 0 R /GS1 6 0 R ... */
  hb_vector_buf_t shading_dict;      /* /SH0 7 0 R ... */
  hb_vector_buf_t xobject_dict;      /* /Im0 8 0 R ... */
  unsigned extgstate_count = 0;
  unsigned shading_count = 0;
  unsigned xobject_count = 0;
  unsigned form_count = 0;

  /* Form XObjects for groups are written while painting, before the
   * resource dictionary is complete, so they refer to it as an
   * object of its own.  Created with the first group; its data is
   * filled in at render time. */
  unsigned resources_obj_id = 0;

  /* Every object is a buffer of its own, so the per-buffer document cap
   * bounds each object but not their number.  Charge all objects against the
   * same cap with a running total, and make the overflow sticky like
   * hb_vector_buf_t::overflowed, so a COLRv1 font that paints one large
   * gradient thousands of times cannot allocate gigabytes of objects that
   * would only be refused when they are concatenated at render time. */
  unsigned objects_size = 0;
  bool overflowed = false;

  /* The transform and clip stack, as the content stream's q/Q pairs
   * nest them.  Kept so that a sweep gradient can be sized to what
   * is visible of it.  Transforms and clip bounds are in root output
   * space. */
  struct gstate_t
  {
    hb_transform_t<> ctm = HB_TRANSFORM_IDENTITY;
    /* This level's own clip. */
    hb_bounds_t<> clip;
  };
  hb_vector_t<gstate_t> gstates;
  /* Pushes that did not fit in gstates; while non-zero the stack
   * does not describe the content stream. */
  unsigned gstates_lost = 0;
  /* Bounds of the clip path being streamed. */
  hb_extents_t<> clip_path_extents;

  hb_transform_t<> current_ctm () const
  {
    if (gstates.length) return gstates.tail ().ctm;
    return HB_TRANSFORM_IDENTITY;
  }

  void push_gstate (const gstate_t &g)
  {
    if (unlikely (gstates_lost || !gstates.push_or_fail (g)))
      gstates_lost++;
  }
  /* Push a clip whose bounds, in the current space, are e. */
  void push_clip (hb_extents_t<> e)
  {
    gstate_t g;
    g.ctm = current_ctm ();
    if (e.is_empty ())
      g.clip = hb_bounds_t<> (hb_bounds_t<>::EMPTY);
    else
    {
      g.ctm.transform_extents (e);
      g.clip = hb_bounds_t<> (e);
    }
    push_gstate (g);
  }
  void pop_gstate ()
  {
    if (unlikely (gstates_lost))
      gstates_lost--;
    else if (gstates.length)
      gstates.pop ();
  }

  bool in_error () const
  { return objects.in_error () || overflowed; }

  unsigned add_object (hb_vector_buf_t &&obj_data)
  {
    /* Per-object framing emitted at render time: "N 0 obj\n", "\nendobj\n",
     * and the xref entry.  Neither operand can exceed the cap, so the sum
     * cannot wrap. */
    const unsigned framing = 17 + 8 + 20;
    if (unlikely (in_error () || obj_data.in_error () ||
		  objects_size + obj_data.length + framing > HB_VECTOR_MAX_DOCUMENT_SIZE))
    {
      overflowed = true;
      return 0; /* Never a valid object id. */
    }
    objects_size += obj_data.length + framing;

    unsigned id = 5 + objects.length; /* objects 1-4 are fixed */
    hb_pdf_obj_t obj;
    obj.data = std::move (obj_data);
    objects.push (std::move (obj));
    return id;
  }

  /* Add ExtGState for fill opacity, return resource name index. */
  unsigned add_extgstate_alpha (float alpha)
  {
    unsigned idx = extgstate_count++;
    hb_vector_buf_t obj;
    obj.append_str ("<< /Type /ExtGState /ca ");
    obj.append_num (alpha, 4);
    obj.append_str (" >>");
    unsigned obj_id = add_object (std::move (obj));

    extgstate_dict.append_str ("/GS");
    extgstate_dict.append_unsigned (idx);
    extgstate_dict.append_c (' ');
    extgstate_dict.append_unsigned (obj_id);
    extgstate_dict.append_str (" 0 R ");
    return idx;
  }

  /* Add ExtGState for blend mode, return resource name index. */
  unsigned add_extgstate_blend (const char *bm)
  {
    unsigned idx = extgstate_count++;
    hb_vector_buf_t obj;
    obj.append_str ("<< /Type /ExtGState /BM /");
    obj.append_str (bm);
    obj.append_str (" >>");
    unsigned obj_id = add_object (std::move (obj));

    extgstate_dict.append_str ("/GS");
    extgstate_dict.append_unsigned (idx);
    extgstate_dict.append_c (' ');
    extgstate_dict.append_unsigned (obj_id);
    extgstate_dict.append_str (" 0 R ");
    return idx;
  }

  /* Add a Form XObject that is an isolated transparency group with
   * @stream as its content, to be painted with `/FmN Do`.  Stores
   * the resource name index in @idx. */
  bool add_form_group (const hb_vector_buf_t &stream, unsigned *idx)
  {
    if (!resources_obj_id)
    {
      hb_vector_buf_t placeholder;
      resources_obj_id = add_object (std::move (placeholder));
      if (unlikely (!resources_obj_id))
	return false;
    }

    /* The form is painted where its content would have been, so it
     * inherits the transform and clip in effect there.  Its bounds
     * in that space are not known; use the same all-covering box as
     * the unbounded fills. */
    hb_vector_buf_t form;
    form.append_str ("<< /Type /XObject /Subtype /Form\n");
    form.append_str ("/BBox [-32767 -32767 32767 32767]\n");
    form.append_str ("/Group << /S /Transparency /CS /DeviceRGB /I true >>\n");
    form.append_str ("/Resources ");
    form.append_unsigned (resources_obj_id);
    form.append_str (" 0 R\n/Length ");
    form.append_unsigned (stream.length);
    form.append_str (" >>\nstream\n");
    form.append_len (stream.arrayZ, stream.length);
    form.append_str ("endstream");
    unsigned form_id = add_object (std::move (form));
    if (unlikely (!form_id))
      return false;

    *idx = form_count++;
    xobject_dict.append_str ("/Fm");
    xobject_dict.append_unsigned (*idx);
    xobject_dict.append_c (' ');
    xobject_dict.append_unsigned (form_id);
    xobject_dict.append_str (" 0 R ");
    return true;
  }

  /* Add ExtGState with an SMask (soft mask) referencing a Form XObject
   * that paints a DeviceGray shading.  Returns the ExtGState resource index. */
  unsigned add_extgstate_smask (unsigned alpha_shading_id,
				float bbox_x, float bbox_y,
				float bbox_w, float bbox_h,
				unsigned precision)
  {
    /* Form XObject: transparency group painting the alpha shading. */
    hb_vector_buf_t form_stream;
    form_stream.append_str ("/SHa sh\n");

    hb_vector_buf_t form;
    form.append_str ("<< /Type /XObject /Subtype /Form\n");
    form.append_str ("/BBox [");
    form.append_num (bbox_x, precision);
    form.append_c (' ');
    form.append_num (bbox_y, precision);
    form.append_c (' ');
    form.append_num (bbox_x + bbox_w, precision);
    form.append_c (' ');
    form.append_num (bbox_y + bbox_h, precision);
    form.append_str ("]\n/Group << /S /Transparency /CS /DeviceGray >>\n");
    form.append_str ("/Resources << /Shading << /SHa ");
    form.append_unsigned (alpha_shading_id);
    form.append_str (" 0 R >> >>\n");
    form.append_str ("/Length ");
    form.append_unsigned (form_stream.length);
    form.append_str (" >>\nstream\n");
    form.append_len (form_stream.arrayZ, form_stream.length);
    form.append_str ("endstream");
    unsigned form_id = add_object (std::move (form));

    /* ExtGState with luminosity soft mask. */
    unsigned idx = extgstate_count++;
    hb_vector_buf_t gs;
    gs.append_str ("<< /Type /ExtGState\n");
    gs.append_str ("/SMask << /Type /Mask /S /Luminosity /G ");
    gs.append_unsigned (form_id);
    gs.append_str (" 0 R >> >>");
    unsigned gs_id = add_object (std::move (gs));

    extgstate_dict.append_str ("/GS");
    extgstate_dict.append_unsigned (idx);
    extgstate_dict.append_c (' ');
    extgstate_dict.append_unsigned (gs_id);
    extgstate_dict.append_str (" 0 R ");
    return idx;
  }

  /* Add a shading, return resource name index. */
  unsigned add_shading (hb_vector_buf_t &&shading_data)
  {
    unsigned obj_id = add_object (std::move (shading_data));
    return add_shading_by_id (obj_id);
  }

  /* Register an already-allocated object as a shading resource. */
  unsigned add_shading_by_id (unsigned obj_id)
  {
    unsigned idx = shading_count++;
    shading_dict.append_str ("/SH");
    shading_dict.append_unsigned (idx);
    shading_dict.append_c (' ');
    shading_dict.append_unsigned (obj_id);
    shading_dict.append_str (" 0 R ");
    return idx;
  }

  /* Add an XObject image, return resource name index.
   * idat_data/idat_len is the concatenated PNG IDAT payload (zlib data).
   * colors counts PNG channels, including alpha; for indexed, pass 1.
   * plte/plte_len is the PLTE chunk data for indexed images (may be null). */
  unsigned add_xobject_png_image (const char *idat_data, unsigned idat_len,
				  unsigned width, unsigned height,
				  unsigned colors, bool has_alpha, int64_t *budget,
				  const uint8_t *plte = nullptr,
				  unsigned plte_len = 0,
				  const uint8_t *trns = nullptr,
				  unsigned trns_len = 0)
  {
    hb_vector_buf_t color_stream, smask_stream;
    if (has_alpha &&
        !hb_pdf_split_png_alpha (&color_stream, &smask_stream,
                                 idat_data, idat_len, width, height, colors, budget))
      return UINT_MAX;

    unsigned idx = xobject_count++;
    hb_vector_buf_t obj;
    obj.append_str ("<< /Type /XObject /Subtype /Image\n");
    obj.append_str ("/Width ");
    obj.append_unsigned (width);
    obj.append_str (" /Height ");
    obj.append_unsigned (height);
    obj.append_str ("\n/BitsPerComponent 8\n");

    if (plte && plte_len >= 3)
    {
      /* Indexed color: /ColorSpace [/Indexed /DeviceRGB N <hex palette>] */
      unsigned n_entries = plte_len / 3;
      obj.append_str ("/ColorSpace [/Indexed /DeviceRGB ");
      obj.append_unsigned (n_entries - 1);
      obj.append_str (" <");
      for (unsigned i = 0; i < n_entries * 3; i++)
      {
	obj.append_c ("0123456789ABCDEF"[plte[i] >> 4]);
	obj.append_c ("0123456789ABCDEF"[plte[i] & 0xF]);
      }
      obj.append_str (">]\n");
    }
    else
    {
      obj.append_str ("/ColorSpace ");
      unsigned color_channels = has_alpha ? colors - 1 : colors;
      obj.append_str (color_channels == 1 ? "/DeviceGray" : "/DeviceRGB");
      obj.append_c ('\n');
    }

    /* Build a soft mask for explicit alpha or indexed tRNS transparency. */
    unsigned smask_id = 0;
    bool have_smask = has_alpha;
    if (!has_alpha && plte && trns && trns_len)
      have_smask = hb_pdf_build_indexed_smask (&smask_stream, idat_data, idat_len,
                                              width, height, trns, trns_len, budget);
    bool color_compressed = has_alpha && hb_pdf_compress_image_stream (&color_stream, budget);
    bool mask_compressed = have_smask && hb_pdf_compress_image_stream (&smask_stream, budget);
    if (*budget < 0) return UINT_MAX;
    if (have_smask)
    {
      hb_vector_buf_t smask_obj;
      smask_obj.append_str ("<< /Type /XObject /Subtype /Image\n");
      smask_obj.append_str ("/Width ");
      smask_obj.append_unsigned (width);
      smask_obj.append_str (" /Height ");
      smask_obj.append_unsigned (height);
      smask_obj.append_str ("\n/ColorSpace /DeviceGray /BitsPerComponent 8\n");
      if (mask_compressed) smask_obj.append_str ("/Filter /FlateDecode\n");
      smask_obj.append_str ("/Length ");
      smask_obj.append_unsigned (smask_stream.length);
      smask_obj.append_str (" >>\nstream\n");
      smask_obj.append_len (smask_stream.arrayZ, smask_stream.length);
      smask_obj.append_str ("\nendstream");
      /* An oversized indexed mask may be omitted; explicit alpha must not
       * be embedded as color samples or reference a truncated mask. */
      if (likely (!smask_obj.in_error ()))
        smask_id = add_object (std::move (smask_obj));
    }

    if (smask_id)
    {
      obj.append_str ("/SMask ");
      obj.append_unsigned (smask_id);
      obj.append_str (" 0 R\n");
    }

    if (has_alpha && !smask_id) return UINT_MAX;

    if (!has_alpha)
    {
      obj.append_str ("/Filter /FlateDecode\n");
      obj.append_str ("/DecodeParms << /Predictor 15 /Colors ");
      obj.append_unsigned (colors);
      obj.append_str (" /BitsPerComponent 8 /Columns ");
      obj.append_unsigned (width);
      obj.append_str (" >>\n");
    }
    else if (color_compressed)
      obj.append_str ("/Filter /FlateDecode\n");
    const char *image_data = has_alpha ? color_stream.arrayZ : idat_data;
    unsigned image_len = has_alpha ? color_stream.length : idat_len;
    obj.append_str ("/Length ");
    obj.append_unsigned (image_len);
    obj.append_str (" >>\nstream\n");
    obj.append_len (image_data, image_len);
    obj.append_str ("\nendstream");

    unsigned obj_id = add_object (std::move (obj));
    if (!obj_id) return UINT_MAX;

    xobject_dict.append_str ("/Im");
    xobject_dict.append_unsigned (idx);
    xobject_dict.append_c (' ');
    xobject_dict.append_unsigned (obj_id);
    xobject_dict.append_str (" 0 R ");
    return idx;
  }
};

/* Store resources pointer in the paint struct's defs buffer
 * (repurposed — defs is unused for PDF). */
static hb_pdf_resources_t *
hb_pdf_get_resources (hb_vector_paint_t *paint)
{
  if (!paint->defs.length)
  {
    /* First call: allocate and store. */
    auto *res = (hb_pdf_resources_t *) hb_calloc (1, sizeof (hb_pdf_resources_t));
    if (unlikely (!res)) return nullptr;
    new (res) hb_pdf_resources_t ();
    if (unlikely (!paint->defs.resize (sizeof (void *))))
    {
      res->~hb_pdf_resources_t ();
      hb_free (res);
      return nullptr;
    }
    memcpy (paint->defs.arrayZ, &res, sizeof (void *));
  }
  hb_pdf_resources_t *res;
  memcpy (&res, paint->defs.arrayZ, sizeof (void *));
  return res;
}

void
hb_vector_paint_pdf_free_resources (hb_vector_paint_t *paint)
{
  if (paint->defs.length >= sizeof (void *))
  {
    hb_pdf_resources_t *res;
    memcpy (&res, paint->defs.arrayZ, sizeof (void *));
    if (res)
    {
      res->~hb_pdf_resources_t ();
      hb_free (res);
    }
  }
  paint->defs.clear ();
}


/* ---- helpers ---- */


/* Emit a glyph outline as PDF path operators into buf. */
static void
hb_pdf_emit_glyph_path (hb_vector_paint_t *paint,
			 hb_font_t *font,
			 hb_codepoint_t glyph,
			 hb_vector_buf_t *buf,
			 hb_extents_t<> *extents = nullptr)
{
  paint->path.clear ();
  /* Skip the outline extraction when the session work budget is
   * spent; an empty path keeps the document structure intact. */
  if (likely (paint->budget_remaining >= 0))
  {
    hb_vector_path_sink_t sink = {&paint->path, paint->get_precision (),
				 paint->x_scale_factor, paint->y_scale_factor,
				 &paint->budget_remaining, extents};
    hb_font_draw_glyph (font, glyph,
			hb_vector_pdf_path_draw_funcs_get (),
			&sink);
  }
  buf->append_len (paint->path.arrayZ, paint->path.length);
  paint->path.clear ();
}


/* ---- paint callbacks ---- */

static void
hb_pdf_paint_push_transform (hb_paint_funcs_t *,
			     void *paint_data,
			     float xx, float yx,
			     float xy, float yy,
			     float dx, float dy,
			     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  auto &body = paint->current_body ();
  unsigned sprec = body.scale_precision ();
  body.append_str ("q\n");
  body.append_num (xx, sprec);
  body.append_c (' ');
  body.append_num (yx, sprec);
  body.append_c (' ');
  body.append_num (xy, sprec);
  body.append_c (' ');
  body.append_num (yy, sprec);
  body.append_c (' ');
  body.append_num (paint->sx (dx));
  body.append_c (' ');
  body.append_num (paint->sy (dy));
  body.append_str (" cm\n");

  if (auto *res = hb_pdf_get_resources (paint))
  {
    hb_pdf_resources_t::gstate_t g;
    g.ctm = res->current_ctm ();
    g.ctm.multiply ({xx, yx, xy, yy, paint->sx (dx), paint->sy (dy)});
    res->push_gstate (g);
  }
}

static void
hb_pdf_paint_pop_transform (hb_paint_funcs_t *,
			    void *paint_data,
			    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  paint->current_body ().append_str ("Q\n");
  if (auto *res = hb_pdf_get_resources (paint))
    res->pop_gstate ();
}

static void
hb_pdf_paint_fill_glyph (hb_paint_funcs_t *,
			 void *paint_data,
			 hb_codepoint_t glyph,
			 hb_font_t *font,
			 hb_bool_t,
			 hb_color_t c,
			 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  float r = hb_color_get_red (c) / 255.f;
  float g = hb_color_get_green (c) / 255.f;
  float b = hb_color_get_blue (c) / 255.f;
  float a = hb_color_get_alpha (c) / 255.f;

  if (a < 1.f / 255.f)
    return;

  auto &body = paint->current_body ();

  bool scoped = false;
  /* Set alpha via ExtGState if needed, scoped so it does not leak
   * into later paints. */
  if (a < 1.f - 1.f / 512.f)
  {
    auto *res = hb_pdf_get_resources (paint);
    if (res)
    {
      unsigned gs_idx = res->add_extgstate_alpha (a);
      body.append_str ("q\n");
      scoped = true;
      body.append_str ("/GS");
      body.append_unsigned (gs_idx);
      body.append_str (" gs\n");
    }
  }

  body.append_num (r, 4);
  body.append_c (' ');
  body.append_num (g, 4);
  body.append_c (' ');
  body.append_num (b, 4);
  body.append_str (" rg\n");
  hb_pdf_emit_glyph_path (paint, font, glyph, &body);
  body.append_str ("f\n");
  if (scoped)
    body.append_str ("Q\n");
}

static void
hb_pdf_paint_push_clip_glyph (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_codepoint_t glyph,
			      hb_font_t *font,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  auto &body = paint->current_body ();
  body.append_str ("q\n");
  hb_extents_t<> extents;
  hb_pdf_emit_glyph_path (paint, font, glyph, &body, &extents);
  body.append_str ("W n\n");

  if (auto *res = hb_pdf_get_resources (paint))
    res->push_clip (extents);
}

static void
hb_pdf_paint_push_clip_rectangle (hb_paint_funcs_t *,
				  void *paint_data,
				  float xmin, float ymin,
				  float xmax, float ymax,
				  void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  auto &body = paint->current_body ();
  body.append_str ("q\n");
  body.append_num (paint->sx (xmin));
  body.append_c (' ');
  body.append_num (paint->sy (ymin));
  body.append_c (' ');
  body.append_num (paint->sx (xmax - xmin));
  body.append_c (' ');
  body.append_num (paint->sy (ymax - ymin));
  body.append_str (" re W n\n");

  if (auto *res = hb_pdf_get_resources (paint))
    res->push_clip ({hb_min (paint->sx (xmin), paint->sx (xmax)),
		     hb_min (paint->sy (ymin), paint->sy (ymax)),
		     hb_max (paint->sx (xmin), paint->sx (xmax)),
		     hb_max (paint->sy (ymin), paint->sy (ymax))});
}

static hb_draw_funcs_t *
hb_pdf_paint_push_clip_path_start (hb_paint_funcs_t *,
				   void *paint_data,
				   void **draw_data,
				   void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
  {
    *draw_data = nullptr;
    return nullptr;
  }

  auto &body = paint->current_body ();
  body.append_str ("q\n");
  /* Stream path operators straight into the body; end() seals
   * the path with "W n" to turn it into the clip region.
   * Coordinates arrive in font-scale; the sink divides by
   * scale_factor so they land in output space. */
  paint->clip_path_sink = {&body, paint->get_precision (),
			   paint->x_scale_factor,
			   paint->y_scale_factor,
			   &paint->budget_remaining, nullptr};
  /* The clip's bounds are known when the path ends. */
  if (auto *res = hb_pdf_get_resources (paint))
  {
    res->clip_path_extents = hb_extents_t<> {};
    paint->clip_path_sink.extents = &res->clip_path_extents;
    hb_pdf_resources_t::gstate_t g;
    g.ctm = res->current_ctm ();
    res->push_gstate (g);
  }
  *draw_data = &paint->clip_path_sink;
  return hb_vector_pdf_path_draw_funcs_get ();
}

static void
hb_pdf_paint_push_clip_path_end (hb_paint_funcs_t *,
				 void *paint_data,
				 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  paint->current_body ().append_str ("W n\n");

  auto *res = hb_pdf_get_resources (paint);
  if (res && paint->clip_path_sink.extents)
  {
    paint->clip_path_sink.extents = nullptr;
    /* Swap the placeholder pushed at the start for the real clip. */
    res->pop_gstate ();
    res->push_clip (res->clip_path_extents);
  }
}

static void
hb_pdf_paint_pop_clip (hb_paint_funcs_t *,
		       void *paint_data,
		       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  paint->current_body ().append_str ("Q\n");
  if (auto *res = hb_pdf_get_resources (paint))
    res->pop_gstate ();
}

/* Paint a solid color, including alpha via ExtGState. */
static void
hb_pdf_paint_solid_color (hb_vector_paint_t *paint, hb_color_t c)
{
  auto &body = paint->current_body ();

  float r = hb_color_get_red (c) / 255.f;
  float g = hb_color_get_green (c) / 255.f;
  float b = hb_color_get_blue (c) / 255.f;
  float a = hb_color_get_alpha (c) / 255.f;

  if (a < 1.f / 255.f)
    return;

  /* Set alpha via ExtGState if needed. */
  if (a < 1.f - 1.f / 512.f)
  {
    auto *res = hb_pdf_get_resources (paint);
    if (res)
    {
      unsigned gs_idx = res->add_extgstate_alpha (a);
      body.append_str ("/GS");
      body.append_unsigned (gs_idx);
      body.append_str (" gs\n");
    }
  }

  /* Set fill color. */
  body.append_num (r, 4);
  body.append_c (' ');
  body.append_num (g, 4);
  body.append_c (' ');
  body.append_num (b, 4);
  body.append_str (" rg\n");

  /* Paint a huge rect (will be clipped). */
  body.append_str ("-32767 -32767 65534 65534 re f\n");
}

static void
hb_pdf_paint_color (hb_paint_funcs_t *,
		    void *paint_data,
		    hb_bool_t,
		    hb_color_t color,
		    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  hb_pdf_paint_solid_color (paint, color);
}

/* Read a big-endian uint32 from a byte pointer. */
static inline uint32_t
hb_pdf_png_u32 (const uint8_t *p)
{
  return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) |
	 ((uint32_t) p[2] << 8)  | (uint32_t) p[3];
}

static hb_bool_t
hb_pdf_paint_image (hb_paint_funcs_t *,
		    void *paint_data,
		    hb_blob_t *image,
		    unsigned width,
		    unsigned height,
		    hb_tag_t format,
		    float slant HB_UNUSED,
		    hb_glyph_extents_t *extents,
		    void *)
{
  if (format != HB_TAG ('p','n','g',' '))
    return false;
  if (!extents || !width || !height)
    return false;

  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return false;
  auto *res = hb_pdf_get_resources (paint);
  /* Over the document cap: stop emitting resource-backed content. */
  if (unlikely (!res || res->in_error () || paint->budget_remaining < 0))
    return false;

  unsigned len = 0;
  const uint8_t *data = (const uint8_t *) hb_blob_get_data (image, &len);
  if (!data || len < 8)
    return false;

  /* Verify PNG signature. */
  static const uint8_t png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  if (hb_memcmp (data, png_sig, 8) != 0)
    return false;

  /* Parse PNG chunks: extract IHDR and concatenate IDAT payloads. */
  unsigned png_w = 0, png_h = 0;
  unsigned colors = 3; /* default RGB */
  uint8_t color_type = 2;
  bool has_alpha = false;
  const uint8_t *plte_data = nullptr;
  unsigned plte_len = 0;
  const uint8_t *trns_data = nullptr;
  unsigned trns_len = 0;
  hb_vector_buf_t idat;

  unsigned pos = 8;
  /* Invariant: pos <= len (len >= 8 checked above).  All bounds checks below
   * are written to avoid 32-bit wrapping: a malicious chunk_len must not be
   * able to wrap pos + 12 + chunk_len past the end of the buffer. */
  while (len - pos >= 12)
  {
    uint32_t chunk_len = hb_pdf_png_u32 (data + pos);
    uint32_t chunk_type = hb_pdf_png_u32 (data + pos + 4);
    if (chunk_len > len - pos - 12)
      break;
    const uint8_t *chunk_data = data + pos + 8;

    if (chunk_type == 0x49484452u) /* IHDR */
    {
      if (chunk_len < 13) return false;
      png_w = hb_pdf_png_u32 (chunk_data);
      png_h = hb_pdf_png_u32 (chunk_data + 4);
      uint8_t bit_depth = chunk_data[8];
      color_type = chunk_data[9];
      if (bit_depth != 8) return false; /* only 8-bit supported */

      switch (color_type)
      {
      case 0: colors = 1; has_alpha = false; break; /* Grayscale */
      case 2: colors = 3; has_alpha = false; break; /* RGB */
      case 3: colors = 1; has_alpha = false; break; /* Indexed */
      case 4: colors = 2; has_alpha = true;  break; /* Gray+Alpha */
      case 6: colors = 4; has_alpha = true;  break; /* RGBA */
      default: return false;
      }
      if (has_alpha && (chunk_data[10] || chunk_data[11] || chunk_data[12]))
        return false; /* Only noninterlaced PNG filter/compression method 0. */
    }
    else if (chunk_type == 0x504C5445u) /* PLTE */
    {
      plte_data = chunk_data;
      plte_len = chunk_len;
    }
    else if (chunk_type == 0x74524E53u) /* tRNS */
    {
      trns_data = chunk_data;
      trns_len = chunk_len;
    }
    else if (chunk_type == 0x49444154u) /* IDAT */
    {
      idat.append_len ((const char *) chunk_data, chunk_len);
    }
    else if (chunk_type == 0x49454E44u) /* IEND */
      break;

    pos += 12 + chunk_len;
  }

  if (!png_w || !png_h || !idat.length || idat.in_error ())
    return false;

  /* A truecolor PLTE is advisory, not an indexed color space. */
  if (color_type != 3) { plte_data = nullptr; plte_len = 0; }

  unsigned im_idx = res->add_xobject_png_image (idat.arrayZ, idat.length,
						 png_w, png_h,
						 colors, has_alpha, &paint->budget_remaining,
						 plte_data, plte_len,
						 trns_data, trns_len);

  if (im_idx == UINT_MAX) return false;

  /* Emit: save state, set CTM to map image (0,0)-(1,1) to extents, paint. */
  auto &body = paint->current_body ();
  body.append_str ("q\n");

  /* Image space: (0,0) at bottom-left, (1,1) at top-right.
   * We need to map to extents (x_bearing, y_bearing+height) .. (x_bearing+width, y_bearing).
   * Since font coords are Y-up like PDF, y_bearing is the top. */
  float ix = (float) extents->x_bearing;
  float iy = (float) extents->y_bearing + (float) extents->height;
  float iw = (float) extents->width;
  float ih = -(float) extents->height; /* negative because image Y goes up but height is negative in extents */

  body.append_num (paint->sx (iw));
  body.append_str (" 0 0 ");
  body.append_num (paint->sy (ih));
  body.append_c (' ');
  body.append_num (paint->sx (ix));
  body.append_c (' ');
  body.append_num (paint->sy (iy));
  body.append_str (" cm\n");

  body.append_str ("/Im");
  body.append_unsigned (im_idx);
  body.append_str (" Do\nQ\n");

  return true;
}

/* ---- Gradient helpers ---- */

static bool
hb_pdf_gradient_needs_alpha (hb_array_t<const hb_color_stop_t> stops)
{
  for (const auto &s : stops)
    if (hb_color_get_alpha (s.color) != 255)
      return true;
  return false;
}

/* Build a PDF Type 2 (exponential interpolation) function for
 * a single alpha stop pair (DeviceGray, scalar output). */
static void
hb_pdf_build_alpha_interpolation_function (hb_vector_buf_t *obj,
					   float a0, float a1)
{
  obj->append_str ("<< /FunctionType 2 /Domain [0 1] /N 1\n");
  obj->append_str ("/C0 [");
  obj->append_num (a0, 4);
  obj->append_str ("]\n/C1 [");
  obj->append_num (a1, 4);
  obj->append_str ("] >>");
}

/* Build a stitching function (Type 3) for the alpha channel of
 * pre-populated paint->color_stops_scratch (already sorted+normalized). */
static unsigned
hb_pdf_build_alpha_gradient_function_from_stops (hb_pdf_resources_t *res,
						 hb_vector_paint_t *paint)
{
  unsigned count = paint->color_stops_scratch.length;

  if (count < 2)
  {
    float a = count ? hb_color_get_alpha (paint->color_stops_scratch.arrayZ[0].color) / 255.f : 1.f;
    hb_vector_buf_t obj;
    hb_pdf_build_alpha_interpolation_function (&obj, a, a);
    return res->add_object (std::move (obj));
  }

  if (count == 2)
  {
    hb_vector_buf_t obj;
    hb_pdf_build_alpha_interpolation_function (&obj,
      hb_color_get_alpha (paint->color_stops_scratch.arrayZ[0].color) / 255.f,
      hb_color_get_alpha (paint->color_stops_scratch.arrayZ[1].color) / 255.f);
    return res->add_object (std::move (obj));
  }

  hb_vector_t<unsigned> sub_func_ids;
  for (unsigned i = 0; i + 1 < count; i++)
  {
    hb_vector_buf_t sub;
    hb_pdf_build_alpha_interpolation_function (&sub,
      hb_color_get_alpha (paint->color_stops_scratch.arrayZ[i].color) / 255.f,
      hb_color_get_alpha (paint->color_stops_scratch.arrayZ[i + 1].color) / 255.f);
    sub_func_ids.push (res->add_object (std::move (sub)));
  }

  hb_vector_buf_t obj;
  obj.append_str ("<< /FunctionType 3 /Domain [0 1]\n");
  obj.append_str ("/Functions [");
  for (unsigned i = 0; i < sub_func_ids.length; i++)
  {
    if (i) obj.append_c (' ');
    obj.append_unsigned (sub_func_ids.arrayZ[i]);
    obj.append_str (" 0 R");
  }
  obj.append_str ("]\n/Bounds [");
  for (unsigned i = 1; i + 1 < count; i++)
  {
    if (i > 1) obj.append_c (' ');
    obj.append_num (paint->color_stops_scratch.arrayZ[i].offset, 4);
  }
  obj.append_str ("]\n/Encode [");
  for (unsigned i = 0; i + 1 < count; i++)
  {
    if (i) obj.append_c (' ');
    obj.append_str ("0 1");
  }
  obj.append_str ("] >>");
  return res->add_object (std::move (obj));
}

/* Build a PDF Type 2 (exponential interpolation) function for
 * a single color stop pair. */
static void
hb_pdf_build_interpolation_function (hb_vector_buf_t *obj,
				     hb_color_t c0, hb_color_t c1)
{
  obj->append_str ("<< /FunctionType 2 /Domain [0 1] /N 1\n");
  obj->append_str ("/C0 [");
  obj->append_num (hb_color_get_red (c0) / 255.f, 4);
  obj->append_c (' ');
  obj->append_num (hb_color_get_green (c0) / 255.f, 4);
  obj->append_c (' ');
  obj->append_num (hb_color_get_blue (c0) / 255.f, 4);
  obj->append_str ("]\n/C1 [");
  obj->append_num (hb_color_get_red (c1) / 255.f, 4);
  obj->append_c (' ');
  obj->append_num (hb_color_get_green (c1) / 255.f, 4);
  obj->append_c (' ');
  obj->append_num (hb_color_get_blue (c1) / 255.f, 4);
  obj->append_str ("] >>");
}

/* Build a stitching function (Type 3) from pre-populated
 * paint->color_stops_scratch (already sorted+normalized). */
static unsigned
hb_pdf_build_gradient_function_from_stops (hb_pdf_resources_t *res,
					   hb_vector_paint_t *paint)
{
  unsigned count = paint->color_stops_scratch.length;

  if (count < 2)
  {
    /* Single stop: constant function. */
    hb_color_t c = count ? paint->color_stops_scratch.arrayZ[0].color
			 : HB_COLOR (0, 0, 0, 255);
    hb_vector_buf_t obj;
    hb_pdf_build_interpolation_function (&obj, c, c);
    return res->add_object (std::move (obj));
  }

  /* Sort by offset. */
  paint->color_stops_scratch.as_array ().qsort (
    [] (const hb_color_stop_t &a, const hb_color_stop_t &b)
    { return (a.offset > b.offset) - (a.offset < b.offset); });

  if (count == 2)
  {
    /* Two stops: single interpolation function. */
    hb_vector_buf_t obj;
    hb_pdf_build_interpolation_function (&obj,
					  paint->color_stops_scratch.arrayZ[0].color,
					  paint->color_stops_scratch.arrayZ[1].color);
    return res->add_object (std::move (obj));
  }

  /* Multiple stops: create sub-functions and stitch. */
  hb_vector_t<unsigned> sub_func_ids;
  for (unsigned i = 0; i + 1 < count; i++)
  {
    hb_vector_buf_t sub;
    hb_pdf_build_interpolation_function (&sub,
					  paint->color_stops_scratch.arrayZ[i].color,
					  paint->color_stops_scratch.arrayZ[i + 1].color);
    sub_func_ids.push (res->add_object (std::move (sub)));
  }

  /* Stitching function (Type 3). */
  hb_vector_buf_t obj;
  obj.append_str ("<< /FunctionType 3 /Domain [0 1]\n");

  /* Functions array. */
  obj.append_str ("/Functions [");
  for (unsigned i = 0; i < sub_func_ids.length; i++)
  {
    if (i) obj.append_c (' ');
    obj.append_unsigned (sub_func_ids.arrayZ[i]);
    obj.append_str (" 0 R");
  }
  obj.append_str ("]\n");

  /* Bounds. */
  obj.append_str ("/Bounds [");
  for (unsigned i = 1; i + 1 < count; i++)
  {
    if (i > 1) obj.append_c (' ');
    obj.append_num (paint->color_stops_scratch.arrayZ[i].offset, 4);
  }
  obj.append_str ("]\n");

  /* Encode array. */
  obj.append_str ("/Encode [");
  for (unsigned i = 0; i + 1 < count; i++)
  {
    if (i) obj.append_c (' ');
    obj.append_str ("0 1");
  }
  obj.append_str ("] >>");

  return res->add_object (std::move (obj));
}

static void
hb_pdf_paint_linear_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float x0, float y0,
			      float x1, float y1,
			      float x2, float y2,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  auto *res = hb_pdf_get_resources (paint);
  /* Over the document cap: stop emitting resource-backed content. */
  if (unlikely (!res || res->in_error ()))
    return;

  /* Fetch, normalize stops to [0,1], and adjust coordinates. */
  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;

  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);
  /* Reduce COLR's 3-anchor (P0, P1, P2) to the 2-point axis of a Type 2
   * (axial) shading, as the SVG, raster, GPU and cairo backends do. */
  float lx0, ly0, lx1, ly1;
  hb_paint_reduce_linear_anchors (x0, y0, x1, y1, x2, y2,
				  &lx0, &ly0, &lx1, &ly1);
  float gx0 = lx0 + mn * (lx1 - lx0);
  float gy0 = ly0 + mn * (ly1 - ly0);
  float gx1 = lx0 + mx * (lx1 - lx0);
  float gy1 = ly0 + mx * (ly1 - ly0);

  unsigned func_id = hb_pdf_build_gradient_function_from_stops (res, paint);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);
  const char *extend_str = (extend == HB_PAINT_EXTEND_PAD)
			    ? "/Extend [true true]\n" : "";

  /* Build Type 2 (axial) shading — color only. */
  hb_vector_buf_t sh;
  sh.append_str ("<< /ShadingType 2 /ColorSpace /DeviceRGB\n");
  sh.append_str ("/Coords [");
  sh.append_num (paint->sx (gx0));
  sh.append_c (' ');
  sh.append_num (paint->sy (gy0));
  sh.append_c (' ');
  sh.append_num (paint->sx (gx1));
  sh.append_c (' ');
  sh.append_num (paint->sy (gy1));
  sh.append_str ("]\n/Function ");
  sh.append_unsigned (func_id);
  sh.append_str (" 0 R\n");
  sh.append_str (extend_str);
  sh.append_str (">>");

  unsigned sh_idx = res->add_shading (std::move (sh));

  auto &body = paint->current_body ();

  bool needs_alpha = hb_pdf_gradient_needs_alpha (stops);
  if (needs_alpha)
  {
    unsigned alpha_func_id = hb_pdf_build_alpha_gradient_function_from_stops (res, paint);

    hb_vector_buf_t ash;
    ash.append_str ("<< /ShadingType 2 /ColorSpace /DeviceGray\n");
    ash.append_str ("/Coords [");
    ash.append_num (paint->sx (gx0));
    ash.append_c (' ');
    ash.append_num (paint->sy (gy0));
    ash.append_c (' ');
    ash.append_num (paint->sx (gx1));
    ash.append_c (' ');
    ash.append_num (paint->sy (gy1));
    ash.append_str ("]\n/Function ");
    ash.append_unsigned (alpha_func_id);
    ash.append_str (" 0 R\n");
    ash.append_str (extend_str);
    ash.append_str (">>");
    unsigned alpha_sh_id = res->add_object (std::move (ash));

    /* An axial shading is not confined to the rectangle spanned by its axis:
     * it extends perpendicular to the axis, and along it with /Extend, and the
     * RGB fill relies on that.  The soft-mask form clips the alpha shading to
     * its /BBox, so give it a large box covering the whole painted area (as
     * the sweep gradient does) instead of the axis box, which is zero-area for
     * a horizontal or vertical gradient and otherwise crops the fill. */
    const float R = 32767.f;
    float scx = paint->sx (gx0), scy = paint->sy (gy0);
    unsigned gs_idx = res->add_extgstate_smask (alpha_sh_id,
						scx - R - 1, scy - R - 1,
						2 * (R + 1), 2 * (R + 1),
						paint->get_precision ());
    body.append_str ("/GS");
    body.append_unsigned (gs_idx);
    body.append_str (" gs\n");
  }

  body.append_str ("/SH");
  body.append_unsigned (sh_idx);
  body.append_str (" sh\n");
}

/* Gives a radial gradient, whose color line is in
 * paint->color_stops_scratch and goes from 0 to 1, two circles to be
 * written as a shading.  Returns false if nothing is to be painted. */
static bool
hb_pdf_radial_gradient_circles (hb_vector_paint_t *paint,
				hb_color_line_t *color_line,
				float *x0, float *y0, float *r0,
				float *x1, float *y1, float *r1)
{
  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);
  if (!paint->radial_gradient_circles (extend, x0, y0, r0, x1, y1, r1))
    return false;

  /* A cone that starts at its tip, and whose first color goes on
   * before it, where there is nothing: pdfium draws a line through the
   * tip then, across everything.  It does not if the cone starts a
   * hair after its tip, far enough for the radius to be written as
   * more than zero.  The first color then goes on to the tip. */
  float dx = *x1 - *x0, dy = *y1 - *y0;
  if (extend == HB_PAINT_EXTEND_PAD &&
      *r0 == 0.f && *r1 > 0.f && dx * dx + dy * dy >= *r1 * *r1)
  {
    float hair = hb_max (1.f / 256, 0.02f * paint->x_scale_factor / *r1);
    if (hair < 0.5f)
    {
      if (unlikely (!paint->cut_color_stops (hair, 1.f)))
	return false;
      *x0 += hair * dx;
      *y0 += hair * dy;
      *r0 = hair * *r1;
    }
  }
  return true;
}

static void
hb_pdf_paint_radial_gradient (hb_paint_funcs_t *,
			      void *paint_data,
			      hb_color_line_t *color_line,
			      float x0, float y0, float r0,
			      float x1, float y1, float r1,
			      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  auto *res = hb_pdf_get_resources (paint);
  /* Over the document cap: stop emitting resource-backed content. */
  if (unlikely (!res || res->in_error ()))
    return;

  /* Fetch, normalize stops to [0,1], and adjust coordinates. */
  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;

  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);
  float gx0 = x0 + mn * (x1 - x0);
  float gy0 = y0 + mn * (y1 - y0);
  float gr0 = r0 + mn * (r1 - r0);
  float gx1 = x0 + mx * (x1 - x0);
  float gy1 = y0 + mx * (y1 - y0);
  float gr1 = r0 + mx * (r1 - r0);

  if (!hb_pdf_radial_gradient_circles (paint, color_line,
				       &gx0, &gy0, &gr0, &gx1, &gy1, &gr1))
    return;

  unsigned func_id = hb_pdf_build_gradient_function_from_stops (res, paint);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);
  const char *extend_str = (extend == HB_PAINT_EXTEND_PAD)
			    ? "/Extend [true true]\n" : "";

  /* Build Type 3 (radial) shading — color only. */
  hb_vector_buf_t sh;
  sh.append_str ("<< /ShadingType 3 /ColorSpace /DeviceRGB\n");
  sh.append_str ("/Coords [");
  sh.append_num (paint->sx (gx0));
  sh.append_c (' ');
  sh.append_num (paint->sy (gy0));
  sh.append_c (' ');
  sh.append_num (paint->sx (gr0));
  sh.append_c (' ');
  sh.append_num (paint->sx (gx1));
  sh.append_c (' ');
  sh.append_num (paint->sy (gy1));
  sh.append_c (' ');
  sh.append_num (paint->sx (gr1));
  sh.append_str ("]\n/Function ");
  sh.append_unsigned (func_id);
  sh.append_str (" 0 R\n");
  sh.append_str (extend_str);
  sh.append_str (">>");

  unsigned sh_idx = res->add_shading (std::move (sh));

  auto &body = paint->current_body ();

  bool needs_alpha = hb_pdf_gradient_needs_alpha (stops);
  if (needs_alpha)
  {
    unsigned alpha_func_id = hb_pdf_build_alpha_gradient_function_from_stops (res, paint);

    hb_vector_buf_t ash;
    ash.append_str ("<< /ShadingType 3 /ColorSpace /DeviceGray\n");
    ash.append_str ("/Coords [");
    ash.append_num (paint->sx (gx0));
    ash.append_c (' ');
    ash.append_num (paint->sy (gy0));
    ash.append_c (' ');
    ash.append_num (paint->sx (gr0));
    ash.append_c (' ');
    ash.append_num (paint->sx (gx1));
    ash.append_c (' ');
    ash.append_num (paint->sy (gy1));
    ash.append_c (' ');
    ash.append_num (paint->sx (gr1));
    ash.append_str ("]\n/Function ");
    ash.append_unsigned (alpha_func_id);
    ash.append_str (" 0 R\n");
    ash.append_str (extend_str);
    ash.append_str (">>");
    unsigned alpha_sh_id = res->add_object (std::move (ash));

    /* The alpha shading extends beyond the circles with /Extend, and the RGB
     * fill relies on that, so the soft-mask form must cover the whole painted
     * area rather than just the enclosing square of the outer circle (which
     * crops the padded region).  Use a large box as the sweep gradient does. */
    const float R = 32767.f;
    float cx = (gr1 >= gr0) ? gx1 : gx0;
    float cy = (gr1 >= gr0) ? gy1 : gy0;
    float scx = paint->sx (cx), scy = paint->sy (cy);
    unsigned gs_idx = res->add_extgstate_smask (alpha_sh_id,
						scx - R - 1, scy - R - 1,
						2 * (R + 1), 2 * (R + 1),
						paint->get_precision ());
    body.append_str ("/GS");
    body.append_unsigned (gs_idx);
    body.append_str (" gs\n");
  }

  body.append_str ("/SH");
  body.append_unsigned (sh_idx);
  body.append_str (" sh\n");
}


/* Encode a 16-bit big-endian unsigned value into buf. */
static void
hb_pdf_encode_u16 (hb_vector_buf_t *buf, uint16_t v)
{
  char bytes[2] = {(char) (v >> 8), (char) (v & 0xFF)};
  buf->append_len (bytes, 2);
}

/* Encode a coordinate as 16-bit value relative to Decode range. */
static void
hb_pdf_encode_coord (hb_vector_buf_t *buf,
		     float val, float lo, float hi)
{
  float t = (val - lo) / (hi - lo);
  t = hb_clamp (t, 0.f, 1.f);
  hb_pdf_encode_u16 (buf, (uint16_t) (t * 65535.f + 0.5f));
}

/* Encode RGB from hb_color_t as 3 bytes. */
static void
hb_pdf_encode_color_rgb (hb_vector_buf_t *buf, hb_color_t c)
{
  char rgb[3] = {(char) hb_color_get_red (c),
		 (char) hb_color_get_green (c),
		 (char) hb_color_get_blue (c)};
  buf->append_len (rgb, 3);
}

/* Encode alpha from hb_color_t as 1 byte (gray). */
static void
hb_pdf_encode_color_alpha (hb_vector_buf_t *buf, hb_color_t c)
{
  char a = (char) hb_color_get_alpha (c);
  buf->append_len (&a, 1);
}

/* Encode one Coons patch control point. */
static void
hb_pdf_encode_point (hb_vector_buf_t *buf,
		     float x, float y,
		     float xlo, float xhi,
		     float ylo, float yhi)
{
  hb_pdf_encode_coord (buf, x, xlo, xhi);
  hb_pdf_encode_coord (buf, y, ylo, yhi);
}

/* Emit one Coons patch sector into the mesh stream(s).
 * Splits large arcs into sub-patches of max 90°.
 * If alpha_mesh is non-null, emits a parallel DeviceGray
 * patch with the alpha channel. */
static void
hb_pdf_add_sweep_patch (hb_vector_buf_t *mesh,
			hb_vector_buf_t *alpha_mesh,
			float cx, float cy, float R,
			float xlo, float xhi, float ylo, float yhi,
			float a0, hb_color_t c0_in,
			float a1, hb_color_t c1_in)
{
  const float MAX_SECTOR = (float) M_PI / 2.f;

  int num_splits = (int) ceilf (fabsf (a1 - a0) / MAX_SECTOR);
  if (num_splits < 1) num_splits = 1;

  for (int s = 0; s < num_splits; s++)
  {
    float k0 = (float) s / num_splits;
    float k1 = (float) (s + 1) / num_splits;
    float sa0 = a0 + k0 * (a1 - a0);
    float sa1 = a0 + k1 * (a1 - a0);
    hb_color_t sc0 = hb_color_lerp (c0_in, c1_in, k0);
    hb_color_t sc1 = hb_color_lerp (c0_in, c1_in, k1);

    float da = sa1 - sa0;
    float kappa = (4.f / 3.f) * tanf (da / 4.f);

    float cos0 = cosf (sa0), sin0 = sinf (sa0);
    float cos1 = cosf (sa1), sin1 = sinf (sa1);

    /* The patch is a wedge: its inner edge is collapsed onto the
     * center.  The mesh coordinates are 16-bit, too coarse to hold
     * a small inner radius: the inner corners would round to
     * different grid points and leave a hole between the patches. */
    float p0x = cx,            p0y = cy;
    float p3x = cx + R * cos0, p3y = cy + R * sin0;
    float p6x = cx + R * cos1, p6y = cy + R * sin1;
    float p9x = cx,            p9y = cy;

    /* Edge 1: p0→p3, radial straight line. */
    float e1_1x = p0x + (p3x - p0x) / 3.f;
    float e1_1y = p0y + (p3y - p0y) / 3.f;
    float e1_2x = p0x + 2.f * (p3x - p0x) / 3.f;
    float e1_2y = p0y + 2.f * (p3y - p0y) / 3.f;

    /* Edge 2: p3→p6, outer arc. */
    float e2_1x = p3x + kappa * R * (-sin0);
    float e2_1y = p3y + kappa * R * ( cos0);
    float e2_2x = p6x - kappa * R * (-sin1);
    float e2_2y = p6y - kappa * R * ( cos1);

    /* Edge 3: p6→p9, radial straight line. */
    float e3_1x = p6x + (p9x - p6x) / 3.f;
    float e3_1y = p6y + (p9y - p6y) / 3.f;
    float e3_2x = p6x + 2.f * (p9x - p6x) / 3.f;
    float e3_2y = p6y + 2.f * (p9y - p6y) / 3.f;

    /* Edge 4: p9→p0, collapsed onto the center. */
    float e4_1x = cx, e4_1y = cy;
    float e4_2x = cx, e4_2y = cy;

    mesh->append_c ('\0'); /* flag = 0, new patch */

    hb_pdf_encode_point (mesh, p0x, p0y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e1_1x, e1_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e1_2x, e1_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p3x, p3y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e2_1x, e2_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e2_2x, e2_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p6x, p6y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e3_1x, e3_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e3_2x, e3_2y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, p9x, p9y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e4_1x, e4_1y, xlo, xhi, ylo, yhi);
    hb_pdf_encode_point (mesh, e4_2x, e4_2y, xlo, xhi, ylo, yhi);

    hb_pdf_encode_color_rgb (mesh, sc0); /* inner start */
    hb_pdf_encode_color_rgb (mesh, sc0); /* outer start */
    hb_pdf_encode_color_rgb (mesh, sc1); /* outer end */
    hb_pdf_encode_color_rgb (mesh, sc1); /* inner end */

    if (alpha_mesh)
    {
      alpha_mesh->append_c ('\0');

      hb_pdf_encode_point (alpha_mesh, p0x, p0y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e1_1x, e1_1y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e1_2x, e1_2y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, p3x, p3y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e2_1x, e2_1y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e2_2x, e2_2y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, p6x, p6y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e3_1x, e3_1y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e3_2x, e3_2y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, p9x, p9y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e4_1x, e4_1y, xlo, xhi, ylo, yhi);
      hb_pdf_encode_point (alpha_mesh, e4_2x, e4_2y, xlo, xhi, ylo, yhi);

      hb_pdf_encode_color_alpha (alpha_mesh, sc0);
      hb_pdf_encode_color_alpha (alpha_mesh, sc0);
      hb_pdf_encode_color_alpha (alpha_mesh, sc1);
      hb_pdf_encode_color_alpha (alpha_mesh, sc1);
    }
  }
}

/* The radius a sweep gradient centered at (cx, cy) has to reach to
 * cover everything the current clips let through: zero if that is
 * nothing, and the largest radius the mesh can encode if the clips
 * do not bound it.
 *
 * The mesh is no bigger than it has to be because viewers resolve a
 * patch, or the whole mesh, in a fixed number of steps whatever its
 * size: around a glyph a thousand units across, patches reaching
 * 32767 units come out as a handful of flat facets. */
static float
hb_pdf_sweep_radius (hb_pdf_resources_t *res,
		     float cx, float cy)
{
  const float MAX_R = 32767.f;

  if (unlikely (res->gstates_lost))
    return MAX_R;

  hb_bounds_t<> bounds;
  for (const auto &g : res->gstates)
    bounds.intersect (g.clip);

  if (bounds.status == hb_bounds_t<>::EMPTY)
    return 0.f;
  if (bounds.status == hb_bounds_t<>::UNBOUNDED)
    return MAX_R;

  /* Bring the corners of the bounds into the gradient's space. */
  hb_transform_t<> t = res->current_ctm ();
  float det = t.xx * t.yy - t.xy * t.yx;
  if (!(fabsf (det) > 1e-12f))
    return MAX_R;

  const hb_extents_t<> &e = bounds.extents;
  float xs[2] = {e.xmin, e.xmax};
  float ys[2] = {e.ymin, e.ymax};
  float r2 = 0.f;
  for (unsigned i = 0; i < 4; i++)
  {
    float dx = xs[i & 1] - t.x0, dy = ys[i >> 1] - t.y0;
    float x = ( t.yy * dx - t.xy * dy) / det - cx;
    float y = (-t.yx * dx + t.xx * dy) / det - cy;
    r2 = hb_max (r2, x * x + y * y);
  }

  float r = ceilf (sqrtf (r2)) + 1.f;
  /* Catches a NaN, too. */
  if (!(r <= MAX_R))
    return MAX_R;
  return r;
}

/* Callback context + trampoline for hb_paint_sweep_gradient_tiles. */
struct hb_pdf_sweep_ctx_t {
  hb_vector_buf_t *mesh;
  hb_vector_buf_t *alpha_mesh;
  float cx, cy, R, xlo, xhi, ylo, yhi;
};

static void
hb_pdf_sweep_emit_patch (float a0, hb_color_t c0,
			 float a1, hb_color_t c1,
			 void *user_data)
{
  auto *ctx = (hb_pdf_sweep_ctx_t *) user_data;
  /* Sweep-gradient meshes are not outline-derived, so they are bounded by
   * the output document-size cap, not the outline work budget: stop once a
   * mesh buffer trips its cap so per-gradient patch counts cannot multiply
   * with the paint-graph traversal limits of the font tables driving us. */
  if (unlikely (ctx->mesh->in_error () ||
		(ctx->alpha_mesh && ctx->alpha_mesh->in_error ())))
    return;
  hb_pdf_add_sweep_patch (ctx->mesh, ctx->alpha_mesh,
			  ctx->cx, ctx->cy, ctx->R,
			  ctx->xlo, ctx->xhi, ctx->ylo, ctx->yhi,
			  a0, c0, a1, c1);
}

static void
hb_pdf_paint_sweep_gradient (hb_paint_funcs_t *,
			     void *paint_data,
			     hb_color_line_t *color_line,
			     float cx, float cy,
			     float start_angle,
			     float end_angle,
			     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  auto *res = hb_pdf_get_resources (paint);
  /* Over the document cap: stop emitting resource-backed content. */
  if (unlikely (!res || res->in_error ()))
    return;

  /* Get and sort color stops. */
  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  stops.as_array ().qsort (
    [] (const hb_color_stop_t &a, const hb_color_stop_t &b)
    { return (a.offset > b.offset) - (a.offset < b.offset); });

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  float scx = paint->sx (cx), scy = paint->sy (cy);
  float R = hb_pdf_sweep_radius (res, scx, scy);
  if (R <= 0.f)
    return;
  /* Whole numbers, so that the /Decode array, written to two
   * decimals, says exactly what the mesh was encoded against. */
  float xlo = floorf (scx - R - 1), xhi = ceilf (scx + R + 1);
  float ylo = floorf (scy - R - 1), yhi = ceilf (scy + R + 1);

  bool needs_alpha = hb_pdf_gradient_needs_alpha (stops);

  hb_vector_buf_t mesh;
  hb_vector_buf_t alpha_mesh;
  mesh.alloc (256);
  if (needs_alpha)
    alpha_mesh.alloc (256);

  hb_pdf_sweep_ctx_t ctx { &mesh, needs_alpha ? &alpha_mesh : nullptr,
			    scx, scy, R, xlo, xhi, ylo, yhi };
  hb_paint_sweep_gradient_tiles (stops.arrayZ, stops.length, extend,
				 start_angle, end_angle,
				 hb_pdf_sweep_emit_patch, &ctx);

  if (!mesh.length)
    return;

  auto hb_pdf_build_mesh_shading = [&] (hb_vector_buf_t &m,
					 const char *cs,
					 const char *decode_suffix) -> unsigned
  {
    hb_vector_buf_t sh;
    sh.append_str ("<< /ShadingType 6 /ColorSpace /");
    sh.append_str (cs);
    sh.append_str ("\n/BitsPerCoordinate 16 /BitsPerComponent 8 /BitsPerFlag 8\n");
    sh.append_str ("/Decode [");
    sh.append_num (xlo, 2);
    sh.append_c (' ');
    sh.append_num (xhi, 2);
    sh.append_c (' ');
    sh.append_num (ylo, 2);
    sh.append_c (' ');
    sh.append_num (yhi, 2);
    sh.append_str (decode_suffix);
    sh.append_str ("]\n/Length ");
    sh.append_unsigned (m.length);
    sh.append_str (" >>\nstream\n");
    sh.append_len (m.arrayZ, m.length);
    sh.append_str ("\nendstream");
    return res->add_object (std::move (sh));
  };

  unsigned sh_obj_id = hb_pdf_build_mesh_shading (mesh, "DeviceRGB",
						   " 0 1 0 1 0 1");
  unsigned sh_idx = res->add_shading_by_id (sh_obj_id);

  auto &body = paint->current_body ();

  if (needs_alpha && alpha_mesh.length)
  {
    unsigned alpha_sh_id = hb_pdf_build_mesh_shading (alpha_mesh, "DeviceGray",
						       " 0 1");
    unsigned gs_idx = res->add_extgstate_smask (alpha_sh_id,
						xlo, ylo,
						xhi - xlo, yhi - ylo,
						paint->get_precision ());
    body.append_str ("/GS");
    body.append_unsigned (gs_idx);
    body.append_str (" gs\n");
  }

  body.append_str ("/SH");
  body.append_unsigned (sh_idx);
  body.append_str (" sh\n");
}

static const char *
hb_pdf_blend_mode_name (hb_paint_composite_mode_t mode)
{
  switch (mode)
  {
  case HB_PAINT_COMPOSITE_MODE_MULTIPLY:       return "Multiply";
  case HB_PAINT_COMPOSITE_MODE_SCREEN:         return "Screen";
  case HB_PAINT_COMPOSITE_MODE_OVERLAY:        return "Overlay";
  case HB_PAINT_COMPOSITE_MODE_DARKEN:         return "Darken";
  case HB_PAINT_COMPOSITE_MODE_LIGHTEN:        return "Lighten";
  case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE:    return "ColorDodge";
  case HB_PAINT_COMPOSITE_MODE_COLOR_BURN:     return "ColorBurn";
  case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT:     return "HardLight";
  case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT:     return "SoftLight";
  case HB_PAINT_COMPOSITE_MODE_DIFFERENCE:     return "Difference";
  case HB_PAINT_COMPOSITE_MODE_EXCLUSION:      return "Exclusion";
  case HB_PAINT_COMPOSITE_MODE_HSL_HUE:        return "Hue";
  case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION: return "Saturation";
  case HB_PAINT_COMPOSITE_MODE_HSL_COLOR:      return "Color";
  case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY: return "Luminosity";
  /* Porter-Duff modes have no PDF blend-mode equivalent; approximate
   * the two that have a plausible color-blend analog, and let the
   * rest fall through to Normal (SRC_OVER). */
  case HB_PAINT_COMPOSITE_MODE_PLUS:           return "Screen";
  case HB_PAINT_COMPOSITE_MODE_XOR:            return "Difference";
  case HB_PAINT_COMPOSITE_MODE_CLEAR:
  case HB_PAINT_COMPOSITE_MODE_SRC:
  case HB_PAINT_COMPOSITE_MODE_DEST:
  case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
  case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
  case HB_PAINT_COMPOSITE_MODE_SRC_IN:
  case HB_PAINT_COMPOSITE_MODE_DEST_IN:
  case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
  case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
  case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
  case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
  default:                                     return nullptr; /* Normal */
  }
}

/* COLR composites a group onto what the enclosing group has painted
 * so far, and onto nothing else.  In PDF that is an isolated
 * transparency group: each group is collected in a buffer of its
 * own and written as a Form XObject, which pop_group paints with
 * the blend mode.  Writing the group's content inline instead would
 * blend it with everything already on the page. */
static void
hb_pdf_paint_push_group (hb_paint_funcs_t *,
			 void *paint_data,
			 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  /* The group becomes an object of the same document, so it shares
   * the document cap with everything already held by the open groups. */
  hb_vector_buf_t group;
  group.precision = paint->current_body ().precision;
  group.base = paint->current_body ().base + paint->current_body ().length;
  if (unlikely (!paint->group_stack.push_or_fail (std::move (group))))
    return;
}

static void
hb_pdf_paint_pop_group (hb_paint_funcs_t *,
			void *paint_data,
			hb_paint_composite_mode_t mode,
			void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  if (paint->group_stack.length < 2)
    return;

  hb_vector_buf_t group = paint->group_stack.pop ();
  if (!group.length)
    return;

  auto *res = hb_pdf_get_resources (paint);
  if (unlikely (!res))
    return;
  unsigned form_idx;
  if (unlikely (group.in_error () || !res->add_form_group (group, &form_idx)))
  {
    /* Make render fail rather than give a document missing a group. */
    res->overflowed = true;
    return;
  }

  auto &body = paint->current_body ();
  body.append_str ("q\n");
  const char *bm = hb_pdf_blend_mode_name (mode);
  if (bm)
  {
    unsigned gs_idx = res->add_extgstate_blend (bm);
    body.append_str ("/GS");
    body.append_unsigned (gs_idx);
    body.append_str (" gs\n");
  }
  body.append_str ("/Fm");
  body.append_unsigned (form_idx);
  body.append_str (" Do\nQ\n");
}


/* ---- lazy loader for paint funcs ---- */

static inline void free_static_pdf_paint_funcs ();

static struct hb_pdf_paint_funcs_lazy_loader_t
  : hb_paint_funcs_lazy_loader_t<hb_pdf_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
    hb_paint_funcs_set_push_transform_func (funcs, hb_pdf_paint_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, hb_pdf_paint_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_fill_glyph_func (funcs, hb_pdf_paint_fill_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, hb_pdf_paint_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, hb_pdf_paint_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_start_func (funcs, hb_pdf_paint_push_clip_path_start, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_end_func (funcs, hb_pdf_paint_push_clip_path_end, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, hb_pdf_paint_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, hb_pdf_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, hb_pdf_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, hb_pdf_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, hb_pdf_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, hb_pdf_paint_sweep_gradient, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, hb_pdf_paint_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, hb_pdf_paint_pop_group, nullptr, nullptr);
    hb_paint_funcs_set_custom_palette_color_func (funcs, hb_vector_paint_custom_palette_color, nullptr, nullptr);
    hb_paint_funcs_set_set_budget_func (funcs, hb_vector_paint_set_budget_callback, nullptr, nullptr);
    hb_paint_funcs_set_get_budget_func (funcs, hb_vector_paint_get_budget_callback, nullptr, nullptr);
    hb_paint_funcs_set_get_budget_remaining_func (funcs, hb_vector_paint_get_budget_remaining_callback, nullptr, nullptr);
    hb_paint_funcs_make_immutable (funcs);
    hb_atexit (free_static_pdf_paint_funcs);
    return funcs;
  }
} static_pdf_paint_funcs;

static inline void
free_static_pdf_paint_funcs ()
{
  static_pdf_paint_funcs.free_instance ();
}

hb_paint_funcs_t *
hb_vector_paint_pdf_funcs_get ()
{
  return static_pdf_paint_funcs.get_unconst ();
}


/* ---- render ---- */

hb_blob_t *
hb_vector_paint_render_pdf (hb_vector_paint_t *paint)
{
  if (!paint->has_extents)
    return nullptr;
  if (!paint->group_stack.length ||
      !paint->group_stack.arrayZ[0].length)
    return nullptr;

  hb_vector_buf_t &content = paint->group_stack.arrayZ[0];
  hb_pdf_resources_t *res = hb_pdf_get_resources (paint);
  /* Over the document cap: the content stream refers to dropped objects. */
  if (unlikely (res && res->in_error ()))
    return nullptr;

  float ex = paint->extents.x;
  float ey = paint->extents.y;
  float ew = paint->extents.width;
  float eh = paint->extents.height;

  /* Build background prefix first: a translucent background registers
   * an ExtGState, which must be counted in the object table and listed
   * in the page resources below.  Scope that ExtGState with q/Q so its
   * alpha does not leak into glyph content. */
  hb_vector_buf_t bg_prefix;
  if (hb_color_get_alpha (paint->background))
  {
    float r = hb_color_get_red (paint->background) / 255.f;
    float g = hb_color_get_green (paint->background) / 255.f;
    float b = hb_color_get_blue (paint->background) / 255.f;
    float a = hb_color_get_alpha (paint->background) / 255.f;
    bool scoped = false;
    if (a < 1.f - 1.f / 512.f)
    {
      if (res)
      {
	bg_prefix.append_str ("q\n");
	scoped = true;
	unsigned gs_idx = res->add_extgstate_alpha (a);
	bg_prefix.append_str ("/GS");
	bg_prefix.append_unsigned (gs_idx);
	bg_prefix.append_str (" gs\n");
      }
    }
    bg_prefix.append_num (r, 4);
    bg_prefix.append_c (' ');
    bg_prefix.append_num (g, 4);
    bg_prefix.append_c (' ');
    bg_prefix.append_num (b, 4);
    bg_prefix.append_str (" rg\n");
    bg_prefix.append_num (ex);
    bg_prefix.append_c (' ');
    bg_prefix.append_num (ey);
    bg_prefix.append_c (' ');
    bg_prefix.append_num (ew);
    bg_prefix.append_c (' ');
    bg_prefix.append_num (eh);
    bg_prefix.append_str (" re f\n");
    if (scoped)
      bg_prefix.append_str ("Q\n");
  }
  if (unlikely (res && res->in_error ()))
    return nullptr;

  /* The resource dictionary, for the page and, as an object of its
   * own, for the groups' Form XObjects. */
  hb_vector_buf_t res_dict;
  if (res &&
      (res->extgstate_dict.length || res->shading_dict.length || res->xobject_dict.length))
  {
    res_dict.append_str (" <<");
    if (res->extgstate_dict.length)
    {
      res_dict.append_str (" /ExtGState << ");
      res_dict.append_len (res->extgstate_dict.arrayZ, res->extgstate_dict.length);
      res_dict.append_str (">>");
    }
    if (res->shading_dict.length)
    {
      res_dict.append_str (" /Shading << ");
      res_dict.append_len (res->shading_dict.arrayZ, res->shading_dict.length);
      res_dict.append_str (">>");
    }
    if (res->xobject_dict.length)
    {
      res_dict.append_str (" /XObject << ");
      res_dict.append_len (res->xobject_dict.arrayZ, res->xobject_dict.length);
      res_dict.append_str (">>");
    }
    res_dict.append_str (" >>");
  }
  if (res && res->resources_obj_id)
  {
    auto &obj = res->objects.arrayZ[res->resources_obj_id - 5];
    obj.data.clear ();
    obj.data.append_len (res_dict.arrayZ, res_dict.length);
  }
  if (unlikely (res_dict.in_error () || (res && res->in_error ())))
    return nullptr;

  unsigned num_extra = res ? res->objects.length : 0;
  unsigned total_objects = 4 + num_extra; /* 1-based: 1..total_objects */

  /* Build PDF. */
  hb_vector_buf_t out;
  hb_buf_recover_recycled (paint->recycled_blob, &out);
  out.alloc (content.length + num_extra * 128 + 1024);

  hb_vector_t<unsigned> offsets;
  if (unlikely (!offsets.resize (total_objects)))
    return nullptr;

  out.append_str ("%PDF-1.4\n%\xC0\xC1\xC2\xC3\n");

  /* Object 1: Catalog */
  offsets.arrayZ[0] = out.length;
  out.append_str ("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

  /* Object 2: Pages */
  offsets.arrayZ[1] = out.length;
  out.append_str ("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");

  /* Object 3: Page */
  offsets.arrayZ[2] = out.length;
  out.append_str ("3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [");
  out.append_num (ex);
  out.append_c (' ');
  out.append_num (ey);
  out.append_c (' ');
  out.append_num (ex + ew);
  out.append_c (' ');
  out.append_num (ey + eh);
  out.append_str ("]\n/Contents 4 0 R");

  /* Resources. */
  if (res_dict.length)
  {
    out.append_str ("\n/Resources");
    out.append_len (res_dict.arrayZ, res_dict.length);
  }

  out.append_str (" >>\nendobj\n");

  unsigned stream_len = bg_prefix.length + content.length;

  /* Object 4: Content stream */
  offsets.arrayZ[3] = out.length;
  out.append_str ("4 0 obj\n<< /Length ");
  out.append_unsigned (stream_len);
  out.append_str (" >>\nstream\n");
  out.append_len (bg_prefix.arrayZ, bg_prefix.length);
  out.append_len (content.arrayZ, content.length);
  out.append_str ("endstream\nendobj\n");

  /* Extra objects (functions, shadings, ExtGState). */
  for (unsigned i = 0; i < num_extra; i++)
  {
    offsets.arrayZ[4 + i] = out.length;
    out.append_unsigned (5 + i);
    out.append_str (" 0 obj\n");
    auto &obj = res->objects.arrayZ[i];
    out.append_len (obj.data.arrayZ, obj.data.length);
    out.append_str ("\nendobj\n");
  }

  /* Cross-reference table */
  unsigned xref_offset = out.length;
  out.append_str ("xref\n0 ");
  out.append_unsigned (total_objects + 1);
  out.append_str ("\n0000000000 65535 f \n");
  for (unsigned i = 0; i < total_objects; i++)
  {
    char tmp[21];
    snprintf (tmp, sizeof (tmp), "%010u 00000 n \n", offsets.arrayZ[i]);
    out.append_len (tmp, 20);
  }

  /* Trailer */
  out.append_str ("trailer\n<< /Size ");
  out.append_unsigned (total_objects + 1);
  out.append_str (" /Root 1 0 R >>\nstartxref\n");
  out.append_unsigned (xref_offset);
  out.append_str ("\n%%EOF\n");

  hb_blob_t *blob = hb_buf_blob_from (&paint->recycled_blob, &out);

  return blob;
}
