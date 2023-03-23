#ifndef __LOCK_H__
#define __LOCK_H__

#include "list.h"
#include "rpi-thread.h"

struct semaphore {
    int value;      /* Current value. */
    struct list waiters; /* List of waiting threads. */
};

void sema_init(struct semaphore *, int value);
void sema_down(struct semaphore *);
bool sema_try_down(struct semaphore *);
void sema_up(struct semaphore *);

struct lock {
    struct rpi_thread *holder;
    struct semaphore semaphore;
} lock;

void lock_init(struct lock *);
void lock_acquire(struct lock *);
bool lock_try_acquire(struct lock *);
void lock_release(struct lock *);

#endif
