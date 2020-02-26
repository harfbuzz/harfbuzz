#include "hb-fuzzer.hh"

#include <stdio.h>

int main (int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
  {
    hb_blob_t *blob = hb_blob_create_from_file (argv[i]);

    unsigned int len;
    const char *font_data = hb_blob_get_data (blob, &len);
    printf ("%s%s\n", argv[i], len ? "" : " (note: not found or was empty)");

    LLVMFuzzerTestOneInput ((const uint8_t *) font_data, len);

    hb_blob_destroy (blob);
  }
  return 0;
}
