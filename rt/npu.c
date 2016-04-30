#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "npu.h"

typedef unsigned long long u64;

void * src_start = 0;
void * dst_start = 0;

static void * mapped_ocm = 0;

int npu_map(void) {
  int memfd = open("/dev/ocm", O_RDWR | O_SYNC);
  if (memfd == -1) {
    return -1;
  }

  mapped_ocm = mmap(
    NULL, //map to any user address
    DATA_SIZE,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    memfd,
    DATA_ADDR
  );

  if (mapped_ocm == MAP_FAILED) {
    return -1;
  }

  if(-1 == close(memfd)) {
    return -1;
  }

  src_start = mapped_ocm + SRC_OFFSET;
  dst_start = mapped_ocm + DST_OFFSET;

  return 0;
}

int npu_unmap(void) {
  if(-1 == munmap(mapped_ocm, DATA_SIZE)) {
    return -1;
  }

  src_start = 0;
  dst_start = 0;
 
  return 0; 
}

#define nsec 1
#define usec (nsec * 1000)
#define msec (usec * 1000)
#define sec  (msec * 1000)

struct timespec interval = {
  .tv_sec  = 0,
  .tv_nsec = 1 * nsec,
};

inline void npu() {
  sev();
  //nanosleep(&interval, 0);
  wfe();
  wfe();
}

#define NPU_IOC_MAGIC 'N'

#define DEST(n) _IOW(NPU_IOC_MAGIC, n, int)

static void npu_config_ram(u64 mif[], int lines, int dest, int pe) {
  static int npu_fd = 0;
  if(npu_fd == 0) {
    npu_fd = open("/dev/npu", O_WRONLY | O_SYNC);
    if(npu_fd == -1) {
      perror("open");
      exit(1);
    }
  }

  int err;

  err = ioctl(npu_fd, DEST(dest), pe);
  if(err) {
    perror("ioctl"); exit(1);
  }

  err = write(npu_fd, mif, lines * 8);
  if(err < 0) {
    perror("write"); exit(1);
  }
}

void npu_config(char * config_file) {
  unsigned char dest, pe;
  unsigned short lines;
  int res;
  u64 buf[512];


  int fd = open(config_file, O_RDONLY);
  if(fd == -1) {
    perror("npu_config_open");
  }

  while( (res = read(fd, &dest, 1)) > 0) {
    if(res < 0) { perror("read"); exit(1); }

    res = read(fd, &pe, 1);
    if(res < 0) { perror("read"); exit(1); }

    res = read(fd, &lines, 2);
    if(res < 0) { perror("read"); exit(1); }

    if(lines < 0 || lines > 512) {
      fprintf(stderr, "npu_config: invalid line count\n");
      exit(1);
    }

    res = read(fd, buf, lines*8);
    if(res < 0) { perror("read"); exit(1); }

    npu_config_ram(buf, lines, dest, pe);
  }
}

