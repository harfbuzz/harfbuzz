#include "hb-fuzzer.hh"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

/* Globals defined in hb-depend-fuzzer.cc */
extern const char *_hb_depend_fuzzer_current_font_path;
extern unsigned _hb_depend_fuzzer_seed;
extern uint64_t _hb_depend_fuzzer_single_test;  /* Specific test to run, or 0 for all */
extern unsigned _hb_depend_fuzzer_num_tests;    /* Number of tests to run (default 1024) */
extern bool _hb_depend_fuzzer_verbose;
extern bool _hb_depend_fuzzer_report_over_approximation;
extern bool _hb_depend_fuzzer_report_under_approximation;
extern float _hb_depend_fuzzer_inject_over_approx;
extern float _hb_depend_fuzzer_inject_under_approx;

/* Explicit test mode */
extern bool _hb_depend_fuzzer_explicit_test;
extern const char *_hb_depend_fuzzer_explicit_unicodes;
extern const char *_hb_depend_fuzzer_explicit_gids;
extern bool _hb_depend_fuzzer_explicit_no_layout_closure;
extern const char *_hb_depend_fuzzer_explicit_layout_features;

static void
print_usage (const char *prog)
{
  fprintf (stderr, "Usage: %s [OPTIONS] FONT...\n", prog);
  fprintf (stderr, "\n");
  fprintf (stderr, "Validate depend API by comparing closures against subset.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Test Modes:\n");
  fprintf (stderr, "  (default)             Run random tests using seed\n");
  fprintf (stderr, "  -t, --test HEXSEED    Run only specific test (64-bit hex, with or without 0x)\n");
  fprintf (stderr, "  --unicodes LIST       Run explicit test with given unicodes (U+XXXX or decimal)\n");
  fprintf (stderr, "  --gids LIST           Run explicit test with given glyph IDs (decimal)\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Explicit Test Options (used with --unicodes or --gids):\n");
  fprintf (stderr, "  --no-layout-closure   Skip GSUB closure (use with --gids)\n");
  fprintf (stderr, "  --layout-features=LIST\n");
  fprintf (stderr, "                        Specify GSUB features (comma-separated tags or * for all)\n");
  fprintf (stderr, "                        Default: subset's default features\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "General Options:\n");
  fprintf (stderr, "  -s, --seed SEED       Use specific random seed (default: random)\n");
  fprintf (stderr, "  -n, --num-tests N     Run N tests per font (default: 1024)\n");
  fprintf (stderr, "  -v, --verbose         Enable verbose output (all test details)\n");
  fprintf (stderr, "  -r, --report-over-approximation\n");
  fprintf (stderr, "                        Report when depend finds more glyphs than subset\n");
  fprintf (stderr, "  --report-under-approximation\n");
  fprintf (stderr, "                        Report under-approximation instead of aborting\n");
  fprintf (stderr, "  -o, --inject-over-approx[=RATE]\n");
  fprintf (stderr, "                        Inject over-approximation errors (default rate: 0.02 = 2%%)\n");
  fprintf (stderr, "  -u, --inject-under-approx[=RATE]\n");
  fprintf (stderr, "                        Inject under-approximation errors (default rate: 0.02 = 2%%)\n");
  fprintf (stderr, "  -h, --help            Show this help message\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Examples:\n");
  fprintf (stderr, "  %s fonts/*.ttf                           # Run 1024 tests on each font\n", prog);
  fprintf (stderr, "  %s -n 100 fonts/*.ttf                    # Run 100 tests per font\n", prog);
  fprintf (stderr, "  %s -v fonts/MyFont.ttf                   # Run with verbose output\n", prog);
  fprintf (stderr, "  %s -t 0x0000303900000015 fonts/MyFont.ttf  # Run specific test\n", prog);
  fprintf (stderr, "  %s --unicodes=U+0041,U+0042 fonts/MyFont.ttf  # Test with 'A' and 'B'\n", prog);
  fprintf (stderr, "  %s --gids=10,20,30 --no-layout-closure fonts/MyFont.ttf  # Non-GSUB test\n", prog);
  fprintf (stderr, "  %s --unicodes=65-90 --layout-features=liga,calt fonts/MyFont.ttf  # Custom features\n", prog);
}

int main (int argc, char **argv)
{
  unsigned seed = 0;
  uint64_t single_test = 0;
  unsigned num_tests = 1024;
  bool verbose = false;
  bool report_over_approximation = false;
  bool report_under_approximation = false;
  bool seed_specified = false;
  float inject_over_approx = 0.0f;
  float inject_under_approx = 0.0f;

  /* Explicit test mode */
  bool explicit_test = false;
  const char *explicit_unicodes = NULL;
  const char *explicit_gids = NULL;
  bool explicit_no_layout_closure = false;
  const char *explicit_layout_features = NULL;

  /* Parse options */
  int arg_idx = 1;
  while (arg_idx < argc && argv[arg_idx][0] == '-')
  {
    if (strcmp (argv[arg_idx], "-h") == 0 || strcmp (argv[arg_idx], "--help") == 0)
    {
      print_usage (argv[0]);
      return 0;
    }
    else if (strcmp (argv[arg_idx], "-v") == 0 || strcmp (argv[arg_idx], "--verbose") == 0)
    {
      verbose = true;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "-r") == 0 || strcmp (argv[arg_idx], "--report-over-approximation") == 0)
    {
      report_over_approximation = true;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "--report-under-approximation") == 0)
    {
      report_under_approximation = true;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "-s") == 0 || strcmp (argv[arg_idx], "--seed") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: %s requires an argument\n", argv[arg_idx]);
        print_usage (argv[0]);
        return 1;
      }
      seed = (unsigned) atoi (argv[arg_idx + 1]);
      seed_specified = true;
      arg_idx += 2;
    }
    else if (strcmp (argv[arg_idx], "-n") == 0 || strcmp (argv[arg_idx], "--num-tests") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: %s requires an argument\n", argv[arg_idx]);
        print_usage (argv[0]);
        return 1;
      }
      num_tests = (unsigned) atoi (argv[arg_idx + 1]);
      arg_idx += 2;
    }
    else if (strcmp (argv[arg_idx], "-t") == 0 || strcmp (argv[arg_idx], "--test") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: %s requires an argument\n", argv[arg_idx]);
        print_usage (argv[0]);
        return 1;
      }
      /* Parse 64-bit hex number (with or without 0x prefix) */
      single_test = strtoull (argv[arg_idx + 1], NULL, 16);
      arg_idx += 2;
    }
    else if (strcmp (argv[arg_idx], "-o") == 0 || strcmp (argv[arg_idx], "--inject-over-approx") == 0)
    {
      /* Check if next arg exists and is a number (not another flag) */
      if (arg_idx + 1 < argc && argv[arg_idx + 1][0] != '-')
      {
        inject_over_approx = strtof (argv[arg_idx + 1], NULL);
        arg_idx += 2;
      }
      else
      {
        /* Use default target rate (2%) */
        inject_over_approx = 0.02f;
        arg_idx++;
      }
    }
    else if (strcmp (argv[arg_idx], "-u") == 0 || strcmp (argv[arg_idx], "--inject-under-approx") == 0)
    {
      /* Check if next arg exists and is a number (not another flag) */
      if (arg_idx + 1 < argc && argv[arg_idx + 1][0] != '-')
      {
        inject_under_approx = strtof (argv[arg_idx + 1], NULL);
        arg_idx += 2;
      }
      else
      {
        /* Use default target rate (2%) */
        inject_under_approx = 0.02f;
        arg_idx++;
      }
    }
    else if (strncmp (argv[arg_idx], "--unicodes=", 11) == 0)
    {
      explicit_test = true;
      explicit_unicodes = argv[arg_idx] + 11;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "--unicodes") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: --unicodes requires an argument\n");
        print_usage (argv[0]);
        return 1;
      }
      explicit_test = true;
      explicit_unicodes = argv[arg_idx + 1];
      arg_idx += 2;
    }
    else if (strncmp (argv[arg_idx], "--gids=", 7) == 0)
    {
      explicit_test = true;
      explicit_gids = argv[arg_idx] + 7;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "--gids") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: --gids requires an argument\n");
        print_usage (argv[0]);
        return 1;
      }
      explicit_test = true;
      explicit_gids = argv[arg_idx + 1];
      arg_idx += 2;
    }
    else if (strcmp (argv[arg_idx], "--no-layout-closure") == 0)
    {
      explicit_no_layout_closure = true;
      arg_idx++;
    }
    else if (strncmp (argv[arg_idx], "--layout-features=", 18) == 0)
    {
      explicit_layout_features = argv[arg_idx] + 18;
      arg_idx++;
    }
    else if (strcmp (argv[arg_idx], "--layout-features") == 0)
    {
      if (arg_idx + 1 >= argc)
      {
        fprintf (stderr, "Error: --layout-features requires an argument\n");
        print_usage (argv[0]);
        return 1;
      }
      explicit_layout_features = argv[arg_idx + 1];
      arg_idx += 2;
    }
    else
    {
      fprintf (stderr, "Error: Unknown option %s\n", argv[arg_idx]);
      print_usage (argv[0]);
      return 1;
    }
  }

  /* Check if we have font files */
  if (arg_idx >= argc)
  {
    fprintf (stderr, "Error: No font files specified\n");
    print_usage (argv[0]);
    return 1;
  }

  /* Validate test mode combinations */
  if (explicit_test && single_test != 0)
  {
    fprintf (stderr, "Error: Cannot use both explicit test mode (--unicodes/--gids) and -t/--test\n");
    print_usage (argv[0]);
    return 1;
  }

  if (explicit_unicodes && explicit_gids)
  {
    fprintf (stderr, "Error: Cannot specify both --unicodes and --gids\n");
    print_usage (argv[0]);
    return 1;
  }

  if (!explicit_test && (explicit_no_layout_closure || explicit_layout_features))
  {
    fprintf (stderr, "Warning: --no-layout-closure and --layout-features are ignored without --unicodes or --gids\n");
  }

  /* Use random seed if not specified */
  if (!seed_specified)
    seed = (unsigned) time (NULL);

  /* Set global configuration */
  _hb_depend_fuzzer_seed = seed;
  _hb_depend_fuzzer_single_test = single_test;
  _hb_depend_fuzzer_num_tests = num_tests;
  _hb_depend_fuzzer_verbose = verbose;
  _hb_depend_fuzzer_report_over_approximation = report_over_approximation;
  _hb_depend_fuzzer_report_under_approximation = report_under_approximation;
  _hb_depend_fuzzer_inject_over_approx = inject_over_approx;
  _hb_depend_fuzzer_inject_under_approx = inject_under_approx;

  /* Set explicit test mode configuration */
  _hb_depend_fuzzer_explicit_test = explicit_test;
  _hb_depend_fuzzer_explicit_unicodes = explicit_unicodes;
  _hb_depend_fuzzer_explicit_gids = explicit_gids;
  _hb_depend_fuzzer_explicit_no_layout_closure = explicit_no_layout_closure;
  _hb_depend_fuzzer_explicit_layout_features = explicit_layout_features;

  /* TAP version must be the first line */
  printf ("TAP version 14\n");

  /* Print test mode info */
  if (explicit_test)
  {
    if (explicit_unicodes)
      printf ("# Explicit test mode: --unicodes=%s\n", explicit_unicodes);
    else if (explicit_gids)
      printf ("# Explicit test mode: --gids=%s\n", explicit_gids);

    if (explicit_no_layout_closure)
      printf ("# Using --no-layout-closure\n");
    if (explicit_layout_features)
      printf ("# Using --layout-features=%s\n", explicit_layout_features);
  }
  else if (single_test == 0)
    printf ("# Using seed: %u, running %u tests per font\n", seed, num_tests);
  else
    printf ("# Running single test: 0x%016llx\n", (unsigned long long) single_test);

  /* Process font files */
  int file_count = 0;
  for (int i = arg_idx; i < argc; i++)
  {
    file_count++;
    hb_blob_t *blob = hb_blob_create_from_file_or_fail (argv[i]);
    assert (blob);

    unsigned len = 0;
    const char *font_data = hb_blob_get_data (blob, &len);
    printf ("# %s (%u bytes)\n", argv[i], len);

    /* Set global so fuzzer can log font path on mismatch */
    _hb_depend_fuzzer_current_font_path = argv[i];

    LLVMFuzzerTestOneInput ((const uint8_t *) font_data, len);

    /* Clear global after processing */
    _hb_depend_fuzzer_current_font_path = NULL;

    printf ("ok %d - %s\n", file_count, argv[i]);

    hb_blob_destroy (blob);
  }

  printf ("1..%d\n", file_count);

  return 0;
}
