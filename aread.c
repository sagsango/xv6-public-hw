#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

  static void
print_16B(char * buf)
{
  for (int i = 0; i < 16; i++) {
    printf(1, " %d", buf[i]);
  }
}

  static int
memmatch(char * p1, char * p2, int size)
{
  for (int i = 0; i < size; i++) {
    if (p1[i] != p2[i]) {
      return i;
    }
  }
  return size;
}

int main(int argc, char ** argv)
{
  char * files[2] = {"ls", "README"};
  int fds[2];
  for (int i = 0; i < 2; i++) {
    fds[i] = open(files[i], 0);
    if (fds[i] < 0) {
      printf(1, "open %s failed\n", files[i]);
      exit();
    }
  }
  // allocate four buffers, two for each file
  char buf1[2][16] = {};
  for (int i = 0; i < 2; i++) {
    read(fds[i], buf1[i], 16);
  }

  char buf2[2][16] = {};
  volatile int status[2][2];
  int expect[2][2]; // aread will tell us how many bytes (>= 0) to expect

  // issue requests
  for (int i = 0; i < 2; i++) {
    expect[i][0] = aread(fds[i], buf2[i], 0, 8, &status[i][0]);
    expect[i][1] = aread(fds[i], buf2[i]+8, 8, 8, &status[i][1]);
    if (expect[i][0] != 8 || expect[i][1] != 8) {
      printf(1, "error: aread on file %s returned %d %d\n", files[i], expect[i][0], expect[i][1]);
      exit(); // this can actually cause kernel panic!! How to fix it?
    }
  }
  // wait for completions
  int wt[2][2];
  while (status[0][0] != expect[0][0])
    wt[0][0]++;
  while (status[0][1] != expect[0][1])
    wt[0][1]++;
  while (status[1][0] != expect[1][0])
    wt[1][0]++;
  while (status[1][1] != expect[1][1])
    wt[1][1]++;

  // print results
  printf(1, "wait times: %d %d %d %d\n", wt[0][0], wt[0][1], wt[1][0], wt[1][1]);
  printf(1, "file 1:  read: "); print_16B(buf1[0]); printf(1, "\n");
  printf(1, "file 1: aread: "); print_16B(buf2[0]); printf(1, "\n");
  printf(1, "file 2:  read: "); print_16B(buf1[1]); printf(1, "\n");
  printf(1, "file 2: aread: "); print_16B(buf2[1]); printf(1, "\n");
  close(fds[0]);
  close(fds[1]);

  // test with a very large file
  int fd = open("usertests", 0);
  if (fd < 0) {
    printf(1, "open usertests failed\n");
    exit();
  }
  struct stat st;
  fstat(fd, &st);
  const uint size = st.size;
  char * large1 = malloc(size);
  char * large2 = malloc(size);
  memset(large1, 0, size);
  memset(large2, 1, size);
  read(fd, large1, size);
  volatile int slarge = 0;
  int exp = aread(fd, large2, 0, size, &slarge);
  if (exp != size) {
    printf(1, "error: expecting %d, should be %d\n", exp, size);
    exit();
  }
  int wlarge = 0;
  while (slarge != size)
    wlarge++;
  printf(1, "large file: wait time: %d\n", wlarge);
  int r = memmatch(large1, large2, size);
  if (r < size) {
    printf(1, "read: error: data mismatch at [%d]\n", r);
  } else {
    printf(1, "read: OK!\n");
  }
  exit();
}
