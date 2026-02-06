#include "hb-fuzzer.hh"

#include <glib.h>

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

/* Container for injection rate variables passed to callback */
struct inject_rates_t {
  float *over_approx;
  float *under_approx;
};

/* Glib option parsing callback for optional float arguments with defaults */
static gboolean
parse_inject_rate (const gchar *option_name,
                   const gchar *value,
                   gpointer data,
                   GError **error)
{
  inject_rates_t *rates = (inject_rates_t *) data;
  float *target;

  /* Determine which rate to set based on option name */
  if (g_str_equal (option_name, "-o") || g_str_equal (option_name, "--inject-over-approx"))
    target = rates->over_approx;
  else if (g_str_equal (option_name, "-u") || g_str_equal (option_name, "--inject-under-approx"))
    target = rates->under_approx;
  else
    return FALSE;  /* Unknown option */

  if (value)
    *target = strtof (value, NULL);
  else
    *target = 0.02f;  /* Default 2% */
  return TRUE;
}

static void
print_usage (const char *prog)
{
  fprintf (stderr, "Usage: %s [OPTIONS] FONT...\n", prog);
  fprintf (stderr, "\n");
  fprintf (stderr, "Validate depend API by comparing closures against subset.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Test Modes:\n");
  fprintf (stderr, "  (default)             Run random tests using seed\n");
  fprintf (stderr, "  -t, --test HEXSEED    Run only specific test (64-bit hex with 0x prefix)\n");
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
  gint64 single_test = 0;  /* Use gint64 for glib */
  unsigned num_tests = 1024;
  gboolean verbose = FALSE;
  gboolean report_over_approximation = FALSE;
  gboolean report_under_approximation = FALSE;
  gboolean seed_specified = FALSE;
  float inject_over_approx = 0.0f;
  float inject_under_approx = 0.0f;

  /* Explicit test mode */
  gboolean explicit_test = FALSE;
  const char *explicit_unicodes = NULL;
  const char *explicit_gids = NULL;
  gboolean explicit_no_layout_closure = FALSE;
  const char *explicit_layout_features = NULL;

  /* Injection rate callback data */
  inject_rates_t inject_rates = {&inject_over_approx, &inject_under_approx};

  /* Define glib option entries */
  GOptionEntry test_mode_entries[] =
  {
    {"test", 't', 0, G_OPTION_ARG_INT64, &single_test,
     "Run only specific test (64-bit hex with 0x prefix)", "0xHEXSEED"},
    {"unicodes", 0, 0, G_OPTION_ARG_STRING, &explicit_unicodes,
     "Run explicit test with given unicodes (U+XXXX or decimal)", "LIST"},
    {"gids", 0, 0, G_OPTION_ARG_STRING, &explicit_gids,
     "Run explicit test with given glyph IDs (decimal)", "LIST"},
    {NULL}
  };

  GOptionEntry explicit_test_entries[] =
  {
    {"no-layout-closure", 0, 0, G_OPTION_ARG_NONE, &explicit_no_layout_closure,
     "Skip GSUB closure (use with --gids)", NULL},
    {"layout-features", 0, 0, G_OPTION_ARG_STRING, &explicit_layout_features,
     "Specify GSUB features (comma-separated tags or * for all, default: subset's default features)", "LIST"},
    {NULL}
  };

  GOptionEntry general_entries[] =
  {
    {"seed", 's', 0, G_OPTION_ARG_INT, &seed,
     "Use specific random seed (default: random)", "SEED"},
    {"num-tests", 'n', 0, G_OPTION_ARG_INT, &num_tests,
     "Run N tests per font (default: 1024)", "N"},
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
     "Enable verbose output (all test details)", NULL},
    {"report-over-approximation", 'r', 0, G_OPTION_ARG_NONE, &report_over_approximation,
     "Report when depend finds more glyphs than subset", NULL},
    {"report-under-approximation", 0, 0, G_OPTION_ARG_NONE, &report_under_approximation,
     "Report under-approximation instead of aborting", NULL},
    {"inject-over-approx", 'o', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (gpointer) &parse_inject_rate,
     "Inject over-approximation errors (default rate: 0.02 = 2%)", "[RATE]"},
    {"inject-under-approx", 'u', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (gpointer) &parse_inject_rate,
     "Inject under-approximation errors (default rate: 0.02 = 2%)", "[RATE]"},
    {NULL}
  };

  /* Parse command line options */
  GError *error = NULL;
  GOptionContext *context = g_option_context_new ("FONT... - Validate depend API by comparing closures against subset");

  /* Add main entries (no callbacks, no user_data needed) */
  GOptionGroup *main_group = g_option_group_new (nullptr, nullptr, nullptr, nullptr, nullptr);
  g_option_group_add_entries (main_group, test_mode_entries);
  g_option_context_set_main_group (context, main_group);

  GOptionGroup *explicit_group = g_option_group_new ("explicit", "Explicit Test Options:", "Show explicit test options", NULL, NULL);
  g_option_group_add_entries (explicit_group, explicit_test_entries);
  g_option_context_add_group (context, explicit_group);

  GOptionGroup *general_group = g_option_group_new ("general", "General Options:", "Show general options",
                                                     static_cast<gpointer>(&inject_rates), nullptr);
  g_option_group_add_entries (general_group, general_entries);
  g_option_context_add_group (context, general_group);

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    fprintf (stderr, "Error parsing options: %s\n", error->message);
    g_error_free (error);
    g_option_context_free (context);
    return 1;
  }
  g_option_context_free (context);

  /* Check if we have font files */
  if (argc < 2)
  {
    fprintf (stderr, "Error: No font files specified\n\n");
    print_usage (argv[0]);
    return 1;
  }

  /* Derive seed_specified from whether seed was explicitly set */
  /* Note: glib doesn't directly tell us if an option was set, so we check if seed != 0 */
  seed_specified = (seed != 0);

  /* Derive explicit_test from whether --unicodes or --gids was specified */
  explicit_test = (explicit_unicodes != NULL || explicit_gids != NULL);

  /* Validate test mode combinations */
  if (explicit_test && single_test != 0)
  {
    fprintf (stderr, "Error: Cannot use both explicit test mode (--unicodes/--gids) and -t/--test\n\n");
    print_usage (argv[0]);
    return 1;
  }

  if (explicit_unicodes && explicit_gids)
  {
    fprintf (stderr, "Error: Cannot specify both --unicodes and --gids\n\n");
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
  _hb_depend_fuzzer_single_test = (uint64_t) single_test;
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

  /* Process font files (glib leaves non-option arguments in argv[1..argc-1]) */
  int file_count = 0;
  for (int i = 1; i < argc; i++)
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
