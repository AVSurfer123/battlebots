// engler, cs140e: brain-dead generic queue. don't want to use STL/c++ in the kernel.
#ifndef __Q_H__
#define __Q_H__
#ifndef E
#	error "Client must define the Q datatype <E>"
#endif

#include "rpi-thread.h"

#define BOUNDARY_USEC 10000
typedef struct Q {
    E *head, *tail;
    unsigned cnt;
} Q_t;

// used for iteration.
static E *Q_start(Q_t *q)   { return q->head; }
static E *Q_next(E *e)      { return e->next; }
static unsigned Q_nelem(Q_t *q) { return q->cnt; }

static int Q_empty(Q_t *q)  { 
    if(q->head)
        return 0;
    assert(Q_nelem(q) == 0);
    demand(!q->tail, invalid Q);
    return 1;
}

// remove from front of list.
static E *Q_pop(Q_t *q) {
    demand(q, bad input);

    E *e = q->head;
    if(!e) {
        assert(Q_empty(q));
        return 0;
    }

    q->cnt--;
    q->head = e->next;
    if(!q->head)
        q->tail = 0;
    return e;
}

// insert at tail. (for FIFO)
static void Q_append(Q_t *q, E *e) {
    e->next = 0;
    q->cnt++;
    if(!q->tail) 
        q->head = q->tail = e;
    else {
        q->tail->next = e;
        q->tail = e;
  }
}

// Return >0 if 2nd arg is better than 1st arg, 0 if equal
typedef int (*cmp_fn)(const E *, const E *);

int always_true_cmp(const E *elem1, const E *elem2) { return 1; }

static unsigned cur_time;

int deadline_in_danger(unsigned deadline, unsigned cur_time) {
  if (cur_time > deadline) {
    // Deadline elapsed! Finish immediately.
    // TODO: Add support for killing stale results
    return 1;
  }

  return (deadline - cur_time) < BOUNDARY_USEC;
}

// Implements "earliest deadline first" semantics
int rt_cmp(const E *elem1, const E *elem2) {
  if (elem2->finish_time_us == elem1->finish_time_us) {
    return 0;
  }
  // TODO: Consider using cur_time to gate unnecessary context switches
  if (elem2->finish_time_us < elem1->finish_time_us &&
         deadline_in_danger(elem1->finish_time_us, cur_time)) {
            return 1;
         }
  return -1;
}

static E *Q_find(Q_t *q, cmp_fn cmp) {
  demand(q->cnt > 0, list must be non - empty);
//   output("Got1\n");
  E *best = Q_pop(q);
  cur_time = timer_get_usec();
  // E *best = NULL;
  for (int i = 0; i < q->cnt; i++) {
    // output("Got %d / %d\n", i, q->cnt);
    if (best == NULL) {
        output("NULL best!\n");
    }
    E *new_candidate = Q_pop(q);
    if (new_candidate == NULL) {
        output("NULL new_candidate!\n");
    }
    // th_trace("Tried %d / %d, %d\n", i, q->cnt, cmp(best, new_candidate));
    // if (new_candidate->state != READY) {
    //     output("ILLEGAL: State: %d for tid=%d\n", new_candidate->state, new_candidate->tid);
    // }
    // demand(new_candidate->state == READY, ready list must not contain blocked candidates);
    new_candidate->state = READY; // hacky and (obviously) not thread-safe!
    if (best == NULL || (cmp(best, new_candidate) >= 0)) {
    //   th_trace("%d is better than %d\n", new_candidate->tid, best->tid);
      Q_append(q, best);
      best = new_candidate;
    } else {
      Q_append(q, new_candidate);
    }
}
  demand(best, list must be non - empty);
  return best;
}

// insert at head (for LIFO)
static void Q_push(Q_t *q, E *e) {
    q->cnt++;
    e->next = q->head;
    q->head = e;
    if(!q->tail)
        q->tail = e;
}

// Adds old thread to running queue, and pops new candidate
static E *Q_schedule_simple(E *old_thread, Q_t *runq) {
  if (old_thread != NULL)
    Q_append(runq, old_thread);
  return Q_pop(runq);
}

// // Adds old thread to running queue, and pops new candidate
static E *Q_schedule_rt(E *old_thread, Q_t *runq) {
  // th_trace("Searching to replace %d (%d)\n", old_thread->tid,
  //          old_thread->finish_time_us);
  if (old_thread != NULL)
    Q_append(runq, old_thread);
  // return Q_pop(runq);
  // th_trace("SEARCHING\n");
  // return Q_find(runq, always_true_cmp);
  E *found = Q_find(runq, rt_cmp);
  // th_trace("Replaced with %d (%d)\n", found->tid, found->finish_time_us);
  return found;
}

// // Adds old thread to running queue, and pops new candidate
static E *Q_schedule(E *old_thread, Q_t *runq, schedule_type schedule) {
  E *(*schedule_func)(E * old_thread, Q_t * runq);
  switch (schedule) {
    case SCHEDULE_BASIC:
      schedule_func = Q_schedule_simple;
      break;
    case SCHEDULE_RTOS:
      schedule_func = Q_schedule_rt;
      break;
    default:
      panic("Unknown scheduler!");
}

  return schedule_func(old_thread, runq);
}

static void Q_init(Q_t *q) {
    *q = (Q_t){0};
}

static inline Q_t Q_mk(void) {
    return (Q_t){0};
}

// insert <e_new> after <e>: <e>=<null> means put at head.
static void Q_insert_after(Q_t *q, E *e, E *e_new) {
    if(!e)
        Q_push(q,e_new);
    else if(q->tail == e)
        Q_append(q,e_new);
    else {
        q->cnt++;
        e_new->next = e->next;
        e->next = e_new;
  }
}
#endif
