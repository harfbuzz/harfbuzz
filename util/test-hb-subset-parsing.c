#include <assert.h>
#include <stdio.h>
#include "glib.h"
#include "hb-subset.h"
#include "helper-subset.hh"

static
hb_face_t* open_font(const char* path)
{
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (path);
  g_assert(blob);
  hb_face_t* face = hb_face_create(blob, 0);
  hb_blob_destroy(blob);

  return face;
}

static
gboolean check_parsing(hb_face_t* face, const char* spec, hb_tag_t axis, float exp_min, float exp_def, float exp_max)
{
  printf(">> testing spec: %s\n", spec);
  hb_subset_input_t* input = hb_subset_input_create_or_fail();
  g_assert(input);

  {
    GError* error;
    char *spec_copy = g_strdup (spec);
    gboolean res = parse_instancing_spec(spec_copy, face, input, &error);
    g_free(spec_copy);
    if (!res) {
      hb_subset_input_destroy(input);
      return res;
    }
  }

  float act_min = 0.0, act_def = 0.0, act_max = 0.0;
  hb_bool_t res = hb_subset_input_get_axis_range(input, axis, &act_min, &act_max, &act_def);
  if (!res) {
    hb_subset_input_destroy(input);
    return false;

  }

  g_assert_cmpuint(exp_min, ==, act_min);
  g_assert_cmpuint(exp_def, ==, act_def);
  g_assert_cmpuint(exp_max, ==, act_max);
  
  hb_subset_input_destroy(input);
  return true;
}

static hb_tag_t wght = HB_TAG('w', 'g', 'h', 't');
static hb_tag_t xxxx = HB_TAG('x', 'x', 'x', 'x');

static void
test_parse_instancing_spec (void)
{
  hb_face_t* face = open_font("../test/api/fonts/AdobeVFPrototype-Subset.otf");
  hb_face_t* roboto = open_font("../test/api/fonts/Roboto-Variable.abc.ttf");

  g_assert(check_parsing(face, "wght=300",         wght, 300,  300,  300));
  g_assert(check_parsing(face, "wght=100:200:300", wght, 100,  200,  300));
  g_assert(check_parsing(face, "wght=:500:",       wght,   0,  500, 1000));
  g_assert(check_parsing(face, "wght=::700",       wght,   0,  700,  700));
  g_assert(check_parsing(face, "wght=200::",       wght, 200, 1000, 1000));
  g_assert(check_parsing(face, "wght=200:300:",    wght, 200,  300, 1000));
  g_assert(check_parsing(face, "wght=:300:500",    wght,   0,  300,  500));
  g_assert(check_parsing(face, "wght=300::700",    wght, 300,  700,  700));
  g_assert(check_parsing(face, "wght=300:700",    wght,  300,  700,  700));
  g_assert(check_parsing(face, "wght=:700",       wght,    0,  700,  700));
  g_assert(check_parsing(face, "wght=200:",       wght,  200, 1000, 1000));

  g_assert(check_parsing(face, "wght=200: xxxx=50", wght,  200, 1000, 1000));
  g_assert(check_parsing(face, "wght=200: xxxx=50", xxxx,   50,   50,   50));
  g_assert(check_parsing(face, "wght=200:,xxxx=50", wght,  200, 1000, 1000));
  g_assert(check_parsing(face, "wght=200:,xxxx=50", xxxx,   50,   50,   50));

  g_assert(check_parsing(face, "wght=200,*=drop", wght,  1000, 1000, 1000));
  g_assert(check_parsing(face, "wght=200,*=drop", xxxx,  0, 0, 0));
  g_assert(check_parsing(face, "*=drop,wght=200", wght,  200, 200, 200));
  g_assert(check_parsing(face, "*=drop,wght=200", xxxx,  0, 0, 0));
  g_assert(check_parsing(face, "*=drop,wght=200,xxxx=50", wght,  200, 200, 200));
  g_assert(check_parsing(face, "*=drop,wght=200,xxxx=50", xxxx,  50, 50, 50));
  g_assert(check_parsing(face, "xxxx=50,*=drop,wght=200", wght,  200, 200, 200));
  g_assert(check_parsing(face, "xxxx=50,*=drop,wght=200", xxxx,  0, 0, 0));
  g_assert(check_parsing(face, "*=drop", wght,  1000, 1000, 1000));
  g_assert(check_parsing(face, "*=drop", xxxx,  0, 0, 0));

  g_assert(check_parsing(roboto, "wght=300",         wght, 300,  300,  300));
  g_assert(check_parsing(roboto, "wght=100:200:300", wght, 100,  200,  300));
  g_assert(check_parsing(roboto, "wght=:500:",       wght, 100,  500,  900));
  g_assert(check_parsing(roboto, "wght=::850",       wght, 100,  400,  850));
  g_assert(check_parsing(roboto, "wght=200::",       wght, 200,  400,  900));
  g_assert(check_parsing(roboto, "wght=200:300:",    wght, 200,  300,  900));
  g_assert(check_parsing(roboto, "wght=:300:500",    wght, 100,  300,  500));
  g_assert(check_parsing(roboto, "wght=300::700",    wght, 300,  400,  700));
  g_assert(check_parsing(roboto, "wght=300:700",    wght,  300,  400,  700));
  g_assert(check_parsing(roboto, "wght=:700",       wght,  100,  400,  700));
  g_assert(check_parsing(roboto, "wght=200:",       wght,  200,  400,  900));

  hb_face_destroy(face);
}


int
main (int argc, char **argv)
{
  test_parse_instancing_spec();
  
  return 0;
}
