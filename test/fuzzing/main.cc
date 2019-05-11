#include "hb-fuzzer.hh"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main (int argc, char **argv)
{
  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);

  unsigned int len;
  const char *font_data = hb_blob_get_data (blob, &len);
  if (len == 0)
  {
    printf ("Font not found.\n");
    exit (1);
  }

  for (int i = 1; i < argc; i++)
  {
    printf ("%s\n", argv[i]);
    LLVMFuzzerTestOneInput ((const uint8_t *) font_data, len);
  }

  hb_blob_destroy (blob);

  return 0;
}
