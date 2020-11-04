#pragma once
#include "types.h"

// declare coroutine
struct coroutine;

// Create a new coroutine and append it to the global list of live coroutines.
// The host thread will call co_run() to run the coroutines.
struct coroutine * co_new(void (*func)(void));

// switch to the first available coroutine to run.
// return 1 if some coroutines ran
// return 0 if there is no coroutine available
int co_run(void);

// similar to co_run(), but uses a for-loop to keep the coroutines running
// until there are no live coroutines to run.
// return 1 if some coroutines ran
// return 0 if there is no coroutine available
int co_run_all(void);

// When a coroutine needs to wait for an event, it calls co_yield() to
// surrender the CPU to other coroutines.
// it should be safe to call co_yield from any non-coroutine context (including the host)
// in such case, co_yield should be equivalent to an no-op function.
void co_yield(void);

// When a coroutine has no more jobs to do, it must terminate itself by calling co_exit().
// it makes no sense to co_exit() from a non-coroutine context (do nothing in this case).
void co_exit(void);

// the host thread is responsible of do the cleaning
// it should call co_destroy() on a coroutine once it has exited.
void co_destroy(struct coroutine * co);
