#include "hb-ot-layout-gsubgpos.hh"

int
main ()
{
  hb_face_t *face = hb_face_builder_create ();
  hb_face_set_glyph_count (face, 200 * 512);
  hb_set_t glyphs, expected;
  {
    OT::hb_closure_context_t c (face, &glyphs, nullptr, nullptr);
    for (unsigned i = 0; i < 200; i++)
    {
      unsigned g = (i * 97 % 200) * 512;
      c.add_output (g);
      expected.add (g);
    }
    c.add_output (200 * 512);
    c.add_output (512);
    hb_always_assert (c.get_output ().is_equal (expected));
    c.flush ();
    hb_always_assert (c.get_output ().is_empty ());
    c.add_output (17);
    expected.add (17);
  }
  hb_always_assert (glyphs.is_equal (expected));
  hb_face_destroy (face);
}
