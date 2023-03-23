/*
 * first basic test: will succeed even if you have not implemented context
 * switching as long as you:
 *  - simply have rpi_exit(0) and rpi_yield() return rather than context switch
 * to another thread.
 *  - [even more basic if you have issues: have rpi_fork simply run immediately]
 *
 * this test is useful to make sure you have the rough idea of how the threads
 * should work.  you should rerun when you have your full package working to
 * ensure you get the same result.
 */

#include "rpi.h"
#include "test-header.h"
#include "lock.h"

static unsigned thread_count, thread_sum;

static int finished = 0;

static struct lock test_lock;

// trivial first thread: does not block, explicitly calls exit.
static void thread_code(void *arg) {
  lock_acquire(&test_lock);
  unsigned *x = arg;

  // check tid
  unsigned tid = rpi_cur_thread()->tid;
  trace("acquired lock in thread tid=%d, with x=%d\n", tid, *x);
  demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
         *x + 1);

  // check yield.
  // rpi_yield();
  // trace("pos1\n");
  thread_count++;

  delay_cycles(300000);
  // trace("in thread tid=%d, with x=%d\n", tid, *x);
  // demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
  //        *x + 1);
  // trace("pos2\n");
  // rpi_yield();
  delay_cycles(300000);
  // trace("in thread tid=%d, with x=%d\n", tid, *x);
  // demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
  //        *x + 1);
  // trace("pos3\n");
  thread_sum += *x;
  // rpi_yield();
  delay_cycles(300000);
  // trace("in thread tid=%d, with x=%d\n", tid, *x);
  // demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
  //        *x + 1);
  // trace("pos4\n");
  finished += 1;
  trace("Finished thread tid=%d!\n", tid);
  lock_release(&test_lock);
  rpi_exit(0);
}

void notmain() {
  test_init();

  // change this to increase the number of threads.
  int n = 30;
  trace("about to test summing of n=%d threads\n", n);

  thread_sum = thread_count = 0;
  lock_init(&test_lock);

  unsigned sum = 0;
  unsigned deadline_us = timer_get_usec() + 100000;
  for (int i = 0; i < n; i++) {
    deadline_us -= 0xde;
    int *x = kmalloc(sizeof *x);
    sum += *x = i;
    rpi_fork_rt(thread_code, x, deadline_us);
  }
  rpi_thread_start_preemptive(SCHEDULE_RTOS);

  // no more threads: check.
  trace("count = %d, sum=%d, finished=%d\n", thread_count, thread_sum, finished);
  assert(thread_count == n);
  assert(thread_sum == sum);
  trace("SUCCESS!\n");
}
