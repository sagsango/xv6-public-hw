#include "types.h"
#include "user.h"

int main(int argc, char** argv) {
  if (argc < 4) {
    printf(1,"Usage: mount <fs type> <device> <mount point>\n");
    exit();
  }
  mount(argv[1],argv[2],argv[3]);
  exit();
  return 0;
}
