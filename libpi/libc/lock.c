#include "rpi.h"
#include "lock.h"
#include "demand.h"
#include "rpi-interrupts.h"
#include "rpi-thread.h"

extern rpi_thread_t *cur_thread;

void sema_init(struct semaphore* sema, int value) {
  demand(sema != NULL, sema must be nonnull);

  sema->value = value;
  list_init (&sema->waiters);
}

void sema_down(struct semaphore *sema) {
  demand(sema != NULL, sema must be nonnull);
//   output("Called sema down with thread %d\n", cur_thread->tid);

  system_disable_interrupts();
  if (sema->value <= 0) {
      output("Thread %d failed to acquire lock\n", cur_thread->tid);
      list_push_back(&sema->waiters, &cur_thread->elem);
      cur_thread->state = BLOCKED;
      block_threads();
  }

  while (sema->value <= 0) {
      rpi_yield();
  }

//   system_disable_interrupts();
  sema->value--;
  system_enable_interrupts();  // NOTE: forcibly enables intrs, doesn't just restore old intr level!
//   output("Acquired sema for thread %d\n", cur_thread->tid);
}

bool sema_try_down(struct semaphore* sema) {
  demand(sema != NULL, sema must be nonnull);

  system_disable_interrupts();
  const bool success = sema->value > 0;
  if (success) {
    sema->value--;
  }
  system_enable_interrupts();  // NOTE: forcibly enables intrs, doesn't just restore old intr level!

  return success;
}

void sema_up(struct semaphore* sema) {
  demand(sema != NULL, sema must be nonnull);
//   output("Called sema up with thread %d\n", cur_thread->tid);

  system_disable_interrupts();
  if (!list_empty(&sema->waiters)) {
    rpi_thread_t *unblocked_thread = list_entry(list_pop_front
                                                    (&sema->waiters),
                                                    struct rpi_thread,
                                                    elem);
    unblocked_thread->state = READY;
    // output("Thread %d now unblocked\n", unblocked_thread->tid);
  }

  unblock_threads();

  sema->value++;
  system_enable_interrupts();  // NOTE: forcibly enables intrs, doesn't just restore old intr level!
}

void lock_init(struct lock* lock) {
  demand(lock != NULL, lock must be nonnull);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

void lock_acquire(struct lock* lock) {
  demand(lock != NULL, lock must be nonnull);

  sema_down (&lock->semaphore);
  lock->holder = cur_thread;
}

bool lock_try_acquire(struct lock* lock) {
  demand(lock != NULL, lock must be nonnull);

  bool success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = cur_thread;
  return success;
}

void lock_release(struct lock* lock) {
  demand(lock != NULL, lock must be nonnull);

  lock->holder = NULL;
  sema_up (&lock->semaphore);
}
