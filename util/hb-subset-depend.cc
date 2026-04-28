/*
 * Copyright © 2025  Adobe, Inc.
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
 * Adobe Author(s): Skef Iterum
 */

#include "batch.hh"
#include "face-options.hh"

#include <set>
#include <string>
#include <vector>

struct depend_t : option_parser_t, face_options_t
{
  gboolean flagged_only = false;
  gboolean no_context = false;
  gboolean no_glyph_names = false;
  char *gids_str = nullptr;

  void
  add_options ()
  {
    set_summary ("Show glyph dependency graph.");
    set_description ("Prints the glyph dependency graph extracted from a font file.\n"
                     "For each source glyph, lists the glyphs it can produce or depend on\n"
                     "through GSUB substitutions, composite glyphs, COLR layers, and MATH variants.");

    face_options_t::add_options (this);

    GOptionEntry depend_entries[] =
    {
      {"gids",           0, 0, G_OPTION_ARG_STRING, &this->gids_str,
       "Filter output to specific glyph IDs (e.g. '1,5,10-20')", "GIDs"},
      {"flagged",        0, 0, G_OPTION_ARG_NONE, &this->flagged_only,
       "Show only edges with over-approximation flags set (C or N)", nullptr},
      {"no-context",     0, 0, G_OPTION_ARG_NONE, &this->no_context,
       "Suppress context set output", nullptr},
      {"no-glyph-names", 0, 0, G_OPTION_ARG_NONE, &this->no_glyph_names,
       "Use numeric glyph IDs instead of names", nullptr},
      {nullptr}
    };
    add_group (depend_entries,
               "depend",
               "Dependency graph options:",
               "Options for filtering and formatting the dependency graph output",
               this,
               false);

    GOptionEntry entries[] =
    {
      {G_OPTION_REMAINING, 0, G_OPTION_FLAG_IN_MAIN,
       G_OPTION_ARG_CALLBACK, (gpointer) &collect_rest, nullptr, "[FONT-FILE]"},
      {nullptr}
    };
    add_main_group (entries, this);

    option_parser_t::add_options ();
  }

  static gboolean
  collect_rest (const char *name G_GNUC_UNUSED,
                const char *arg,
                gpointer    data,
                GError    **error)
  {
    depend_t *thiz = (depend_t *) data;

    if (!thiz->font_file)
    {
      thiz->font_file = g_strdup (arg);
      return true;
    }

    g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                 "Too many arguments on the command line");
    return false;
  }

  /* Returns the glyph name if font is non-null and the font has a real name
   * for this glyph; otherwise returns the glyph ID as a decimal string. */
  static std::string
  glyph_str (hb_font_t *font, hb_codepoint_t gid)
  {
    if (font)
    {
      char buf[128];
      if (hb_font_get_glyph_name (font, gid, buf, sizeof (buf)))
        return buf;
    }
    char buf[32];
    snprintf (buf, sizeof (buf), "%u", gid);
    return buf;
  }

  /* Parses a comma-separated list of GID ranges (e.g. "1,5,10-20,42"). */
  static std::set<unsigned>
  parse_gids (const char *str)
  {
    std::set<unsigned> result;
    const char *p = str;
    while (*p)
    {
      char *end;
      unsigned long start = strtoul (p, &end, 10);
      if (end == p) break;
      p = end;
      if (*p == '-')
      {
        p++;
        unsigned long stop = strtoul (p, &end, 10);
        if (end == p) break;
        p = end;
        for (unsigned long i = start; i <= stop; i++)
          result.insert ((unsigned) i);
      }
      else
        result.insert ((unsigned) start);
      if (*p == ',') p++;
    }
    return result;
  }

  /* Builds a human-readable string representing the context set at ctx_idx.
   * Elements < 0x80000000 are direct GID refs joined with &.
   * Elements >= 0x80000000 are indirect set refs (strip high bit for index),
   * printed as parenthesized |-separated members. */
  static std::string
  build_context_str (hb_subset_depend_t *depend, hb_codepoint_t ctx_idx,
                     hb_font_t *font)
  {
    hb_set_t *ctx_set = hb_set_create ();
    if (!hb_subset_depend_lookup_set (depend, ctx_idx, ctx_set))
    {
      hb_set_destroy (ctx_set);
      return "";
    }

    std::string result = "(";
    bool first = true;
    hb_codepoint_t elem = HB_SET_VALUE_INVALID;
    while (hb_set_next (ctx_set, &elem))
    {
      if (!first) result += " & ";
      first = false;
      if (elem < 0x80000000u)
      {
        result += glyph_str (font, elem);
      }
      else
      {
        hb_codepoint_t set_idx = elem & 0x7FFFFFFFu;
        hb_set_t *sub_set = hb_set_create ();
        if (hb_subset_depend_lookup_set (depend, set_idx, sub_set))
        {
          result += "(";
          bool sub_first = true;
          hb_codepoint_t sub_elem = HB_SET_VALUE_INVALID;
          while (hb_set_next (sub_set, &sub_elem))
          {
            if (!sub_first) result += "|";
            sub_first = false;
            result += glyph_str (font, sub_elem);
          }
          result += ")";
        }
        hb_set_destroy (sub_set);
      }
    }
    result += ")";

    hb_set_destroy (ctx_set);
    return result;
  }

  int
  operator () (int argc, char **argv)
  {
    add_options ();

    parse (&argc, &argv);

    hb_subset_depend_t *depend = hb_subset_depend_from_face_or_fail (face);
    if (!depend)
    {
      fprintf (stderr, "Failed to build dependency graph\n");
      return 1;
    }

    hb_font_t *font = no_glyph_names ? nullptr : hb_font_create (face);

    std::set<unsigned> gid_filter;
    bool filter_gids = gids_str != nullptr;
    if (filter_gids)
      gid_filter = parse_gids (gids_str);

    unsigned num_glyphs = hb_face_get_glyph_count (face);
    std::set<hb_codepoint_t> seen_lig_sets;
    bool printed_any = false;

    for (unsigned gid = 0; gid < num_glyphs; gid++)
    {
      if (filter_gids && !gid_filter.count (gid))
        continue;

      unsigned total = hb_subset_depend_lookup_glyph (depend, gid, 0, nullptr, nullptr);
      if (total == 0) continue;

      /* Collect entries, applying the --flagged filter */
      std::vector<hb_subset_depend_entry_t> entries;
      for (unsigned i = 0; i < total; i++)
      {
        hb_subset_depend_entry_t entry;
        unsigned count = 1;
        hb_subset_depend_lookup_glyph (depend, gid, i, &count, &entry);
        if (flagged_only && entry.flags == HB_SUBSET_DEPEND_EDGE_FLAG_NONE)
          continue;
        entries.push_back (entry);
      }

      if (entries.empty ()) continue;

      if (printed_any) printf ("\n");
      printed_any = true;

      /* Source GID header: "42 (glyphname):" or "42:" */
      {
        char name_buf[128];
        if (font && hb_font_get_glyph_name (font, gid, name_buf, sizeof (name_buf)))
          printf ("%u (%s):\n", gid, name_buf);
        else
          printf ("%u:\n", gid);
      }

      for (const auto &entry : entries)
      {
        std::string line = "  ";

        /* Table tag, trailing spaces trimmed (e.g. "CFF " -> "CFF") */
        char tag_buf[5];
        hb_tag_to_string (entry.table_tag, tag_buf);
        tag_buf[4] = 0;
        int tag_end = 4;
        while (tag_end > 0 && tag_buf[tag_end - 1] == ' ') tag_end--;
        tag_buf[tag_end] = 0;
        line += tag_buf;

        /* Feature tag only for GSUB edges */
        if (entry.table_tag == HB_OT_TAG_GSUB)
        {
          char feat_buf[5];
          hb_tag_to_string (entry.layout_tag, feat_buf);
          feat_buf[4] = 0;
          line += " ";
          line += feat_buf;
        }

        line += " -> ";
        line += glyph_str (font, entry.dependent);

        if (entry.ligature_set_index != HB_CODEPOINT_INVALID)
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "  lig[%u]", entry.ligature_set_index);
          line += buf;
        }

        if (entry.flags)
        {
          line += "  flags: ";
          if (entry.flags & HB_SUBSET_DEPEND_EDGE_FLAG_FROM_CONTEXT_POSITION) line += "C";
          if (entry.flags & HB_SUBSET_DEPEND_EDGE_FLAG_FROM_NESTED_CONTEXT)   line += "N";
        }

        /* Context: append inline if it fits within 80 columns, else wrap */
        bool ctx_wraps = false;
        std::string ctx_str;
        if (!no_context && entry.context_set_index != HB_CODEPOINT_INVALID)
        {
          ctx_str = build_context_str (depend, entry.context_set_index, font);
          if (!ctx_str.empty ())
          {
            if (line.size () + 2 + ctx_str.size () <= 80)
              line += "  " + ctx_str;
            else
              ctx_wraps = true;
          }
        }

        printf ("%s\n", line.c_str ());

        if (ctx_wraps)
          printf ("    ctx: %s\n", ctx_str.c_str ());

        /* Ligature set contents: printed once (on first occurrence) */
        if (entry.ligature_set_index != HB_CODEPOINT_INVALID &&
            !seen_lig_sets.count (entry.ligature_set_index))
        {
          seen_lig_sets.insert (entry.ligature_set_index);
          hb_set_t *lig_set = hb_set_create ();
          if (hb_subset_depend_lookup_set (depend, entry.ligature_set_index, lig_set))
          {
            printf ("    lig[%u]:", entry.ligature_set_index);
            hb_codepoint_t lig_gid = HB_SET_VALUE_INVALID;
            while (hb_set_next (lig_set, &lig_gid))
              printf (" %s", glyph_str (font, lig_gid).c_str ());
            printf ("\n");
          }
          hb_set_destroy (lig_set);
        }
      }
    }

    if (font) hb_font_destroy (font);
    hb_subset_depend_destroy (depend);

    return 0;
  }
};

int
main (int argc, char **argv)
{
  return batch_main<depend_t> (argc, argv);
}
