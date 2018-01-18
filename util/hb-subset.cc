#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "hb-private.hh"
#include "hb-blob.h"

int
main (int argc, char **argv)
{
  int exit_code = 0;
  int fd = open("/tmp/Lobster-Regular.ttf", O_RDONLY);
  if (fd == -1) {
    perror("Unable to open file");
    exit(1);
  }

  void *mapped_file = MAP_FAILED;
  char *raw_font = nullptr;

  struct stat stat;
  if (fstat(fd, &stat) != -1) {
    printf("File is %zu bytes\n", stat.st_size);

    void *mapped_file = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_file != MAP_FAILED) {
      raw_font = static_cast<char*>(mapped_file);
    } else {
      perror("Failed to map file");
    }
  } else {
    perror("Unable to fstat");
  }

  if (raw_font) {
      printf("Mapped file\n");      
      for (int i = 0; i < 4; i++) {
        printf("%02x", *(raw_font + i));
      }
      printf("\n");

      hb_blob_t *font_blob = hb_blob_create(raw_font, stat.st_size, HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  }

  if (mapped_file != MAP_FAILED) {
    if (munmap(mapped_file, stat.st_size) == -1) {
      perror("Unable to unmap file");
      exit_code = 1;
    }
  }

  if (close(fd) == -1) {
    perror("Unable to close file");
    exit_code = 1;
  }
  return exit_code;
}
