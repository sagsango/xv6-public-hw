#include "coroutine.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

static volatile int shared_var = 0;
static volatile int locked = 0;

  void
co_lock_worker(void)
{
  const int rank = shared_var++;
  if (locked) {
    int counter = 0;
    do {
      if (counter == 0)
        printf(1, "co[%d] waiting for the lock\n", rank);
      else
        printf(1, "co[%d] the lock is still unavailable :(\n", rank);

      counter++;
      co_yield();
    } while (locked);
  }
  locked = 1;
  printf(1, "co[%d] lock acquired :)\n", rank);
  for (int i = 0; i < 10; i++) { // pretend to be doing something important while holding the lock
    co_yield();
  }
  locked = 0; // unlock
}

  int
main(int argc, char ** argv)
{
  struct coroutine * co[3];
  for (int i = 0; i < 3; i++) {
    co[i] = co_new(co_lock_worker);
    if (!co[i]) {
      printf(1, "host: create co[%d] failed\n");
      exit();
    }
  }
  printf(1, "host: coroutines created!\n");
  if (!co_run_all()) {
    printf(1, "\nhost: error: co_run_all() failed\n");
  }
  for (int i = 0; i < 3; i++) {
    co_destroy(co[i]);
  }
  exit();
}
