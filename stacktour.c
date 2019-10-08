#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

void
rec_print(addr_t a_addr)
{
  int b;
  printf(1, "a %p, b %p, a-b %p\n", a_addr, &b, a_addr - (addr_t)&b);
  rec_print(a_addr);
}


int
main(int argc, char ** argv)
{
  int a;
  rec_print(&a);
  exit();
}
