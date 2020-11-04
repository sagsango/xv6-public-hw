#include "coroutine.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

  void
co_hello(void)
{
  printf(1, "Hello coroutine!\n");
  co_yield();
  printf(1, "Hello Again!\n");
}

  int
main(int argc, char ** argv)
{
  struct coroutine * co = co_new(co_hello);
  printf(1, "host: one coroutine created!\n");
  if (co_run() == 0) {
    printf(1, "host: co_run() failed (1st)\n");
    exit();
  } else {
    printf(1, "host: Expecting one more Hello after this line\n");
  }
  if (co_run() == 0) {
    printf(1, "host: co_run() failed (2nd)\n");
  } else {
    printf(1, "host: coroutine returned\n");
  }
  co_destroy(co);
  exit();
}
