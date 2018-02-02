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

  if (argc != 4) {
    fprintf(stderr, "Must have 4 args\n");
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("Unable to open font file");
    exit(1);
  }

  void *mapped_file = MAP_FAILED;
  int fd_out = -1;

  struct stat stat;
  if (fstat(fd, &stat) != -1) {

    mapped_file = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_file == MAP_FAILED) {
      perror("Failed to map file");
    }
  } else {
    perror("Unable to fstat");
  }

  if (mapped_file != MAP_FAILED) {
    hb_blob_t *font_blob = hb_blob_create(static_cast<const char*>(mapped_file),
                                          stat.st_size,
                                          HB_MEMORY_MODE_READONLY, nullptr,
                                          nullptr);

    fd_out = open(argv[2], O_CREAT | O_WRONLY, S_IRWXU);
    if (fd_out != -1) {
      ssize_t bytes_written = write(fd_out, mapped_file, stat.st_size);
      if (bytes_written == -1) {
        perror("Unable to write output file");
        exit_code = 1;
      } else if (bytes_written != stat.st_size) {
        fprintf(stderr, "Wrong number of bytes written");
        exit_code = 1;
      }
    } else {
      perror("Unable to open output file");
      exit_code = 1;
    }
  }

  if (mapped_file != MAP_FAILED) {
    if (munmap(mapped_file, stat.st_size) == -1) {
      perror("Unable to unmap file");
      exit_code = 1;
    }
  }

  if (fd_out != -1 && close(fd_out) == -1) {
    perror("Unable to close output file");
    exit_code = 1;
  }

  if (fd != -1 && close(fd) == -1) {
    perror("Unable to close file");
    exit_code = 1;
  }

  return exit_code;
}
