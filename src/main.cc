#include "harfbuzz-open-private.h"
#include "harfbuzz-gdef-private.h"

#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  GMappedFile *mf = g_mapped_file_new (argv[1], FALSE, NULL);
  const char *font_data = g_mapped_file_get_contents (mf);
  int len = g_mapped_file_get_length (mf);
  
  printf ("Opened font file %s: %d bytes long\n", argv[1], len);
  
  const OpenTypeFontFile &ot = OpenTypeFontFile::get (font_data);

  switch (ot.get_tag()) {
  case OpenTypeFontFile::TrueTypeTag:
    printf ("OpenType font with TrueType outlines\n");
    break;
  case OpenTypeFontFile::CFFTag:
    printf ("OpenType font with CFF (Type1) outlines\n");
    break;
  case OpenTypeFontFile::TTCTag:
    printf ("TrueType Collection of OpenType fonts\n");
    break;
  default:
    printf ("Unknown font format\n");
    break;
  }

  int num_fonts = ot.get_len ();
  printf ("%d font(s) found in file\n", num_fonts);
  for (int n_font = 0; n_font < num_fonts; n_font++) {
    const OpenTypeFontFace &font = ot[n_font];
    printf ("Font %d of %d:\n", n_font+1, num_fonts);

    int num_tables = font.get_len ();
    printf ("  %d table(s) found in font\n", num_tables);
    for (int n_table = 0; n_table < num_tables; n_table++) {
      const OpenTypeTable &table = font[n_table];
      printf ("  Table %2d of %2d: %.4s (0x%08lx+0x%08lx)\n", n_table+1, num_tables,
	      (const char *)table.get_tag(), table.get_offset(), table.get_length());

      if (table.get_tag() == "GSUB" || table.get_tag() == "GPOS") {
        const GSUBGPOSHeader &g = (const GSUBGPOSHeader&)*ot[table];

	const ScriptList &scripts = *g.get_script_list();
	int num_scripts = scripts.get_len ();
	printf ("    %d script(s) found in table\n", num_scripts);
	for (int n_script = 0; n_script < num_scripts; n_script++) {
	  const Script &script = scripts[n_script];
	  printf ("    Script %2d of %2d: %.4s\n", n_script+1, num_scripts,
	          (const char *)scripts.get_tag(n_script));

	  if (script.get_default_language_system () == NULL)
	    printf ("      No default language system\n");
	  int num_langsys = script.get_len ();
	  printf ("      %d language system(s) found in script\n", num_langsys);
	  for (int n_langsys = 0; n_langsys < num_langsys; n_langsys++) {
	    const LangSys &langsys = script[n_langsys];
	    printf ("      Language System %2d of %2d: %.4s; %d features\n", n_langsys+1, num_langsys,
	            (const char *)script.get_tag(n_langsys),
		    langsys.get_len ());
	    if (!langsys.get_required_feature_index ())
	      printf ("        No required feature\n");
	  }
	}
        
	const FeatureList &features = *g.get_feature_list();
	int num_features = features.get_len ();
	printf ("    %d feature(s) found in table\n", num_features);
	for (int n_feature = 0; n_feature < num_features; n_feature++) {
	  const Feature &feature = features[n_feature];
	  printf ("    Feature %2d of %2d: %.4s; %d lookup(s)\n", n_feature+1, num_features,
	          (const char *)features.get_tag(n_feature),
		  feature.get_len());
	}
        
	const LookupList &lookups = *g.get_lookup_list();
	int num_lookups = lookups.get_len ();
	printf ("    %d lookup(s) found in table\n", num_lookups);
	for (int n_lookup = 0; n_lookup < num_lookups; n_lookup++) {
	  const Lookup &lookup = lookups[n_lookup];
	  printf ("    Lookup %2d of %2d: type %d, flags %04x\n", n_lookup+1, num_lookups,
	          lookup.get_type(), lookup.get_flag());
	}
      }
    }
  }

  return 0;
}
