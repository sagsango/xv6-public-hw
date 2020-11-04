#include "coroutine.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

static int nr_tasks;
static volatile int shared_var = 0;

  void
co_take_turns(void)
{
  const int rank = shared_var++;
  for (int i = 0; i < 6; i++) {
    co_yield();
    if (rank == (shared_var % nr_tasks)) {
      printf(1, " %d", rank);
      shared_var++;
    } else {
      printf(1, "error: shared_var not updated by other coroutines\n");
      co_exit();
    }
  }
}

  int
main(int argc, char ** argv)
{
  struct coroutine * co[6];
  nr_tasks = 6;
  for (int i = 0; i < 6; i++) {
    co[i] = co_new(co_take_turns);
    if (!co[i]) {
      printf(1, "host: create co[%d] failed\n");
      exit();
    }
  }
  printf(1, "host: coroutines created!\n");
  if (co_run_all()) {
    printf(1, "\nhost: co_run_all() returned; shared_var == %d"
        " (expecting %d, the answer to the Ultimate Question of Life, the Universe, and Everything)\n",
        shared_var, nr_tasks * 7);
  } else {
    printf(1, "\nhost: error: co_run_all() failed\n");
  }
  for (int i = 0; i < 6; i++) {
    co_destroy(co[i]);
  }
  exit();
}
