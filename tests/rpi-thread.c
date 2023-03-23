// engler, cs140e: starter code for trivial threads package.
#include "rpi.h"
#include "rpi-thread.h"
#include "rpi-interrupts.h"
#include "timer-interrupt.h"
#include "rpi-armtimer.h"
#include "fast-hash32.h"

// tracing code.  set <trace_p>=0 to stop tracing
enum { trace_p = 1};
#define th_trace(args...) do {                          \
    if(trace_p) {                                       \
        trace(args);                                   \
    }                                                   \
} while(0)
int scheduler_interrupt_cnt = 0;
/***********************************************************************
 * datastructures used by the thread code.
 *
 * you don't have to modify this.
 */

#define E rpi_thread_t
#include "libc/Q.h"

// Set to 1 if, on thread exit, the next thread queued should be the
// highest-priority one. Set to 0 if an arbitrary thread should be chosen, to
// demonstrate the scheduler in action.
#define SCHEDULE_BEST_ON_EXIT 1

// currently only have a single run queue and a free queue.
// the run queue is FIFO.
Q_t runq, freeq, blockq;
rpi_thread_t *cur_thread;        // current running thread.
static rpi_thread_t *scheduler_thread;  // first scheduler thread.

// monotonically increasing thread id: won't wrap before reboot :)
static unsigned tid = 1;
static int preemption = 0;

static schedule_type schedule = SCHEDULE_BASIC;

/***********************************************************************
 * simplistic pool of thread blocks: used to make alloc/free faster (plus,
 * our kmalloc doesn't have free (other than reboot).
 *
 * you don't have to modify this.
 */

// total number of thread blocks we have allocated.
static unsigned nalloced = 0;

// keep a cache of freed thread blocks.  call kmalloc if run out.
rpi_thread_t *th_alloc(void) {
    rpi_thread_t *t = Q_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
#   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
    demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    return t;
}

static void th_free(rpi_thread_t *th) {
    // push on the front in case helps with caching.
    Q_push(&freeq, th);
}

void unblock_threads(void) {
  for (int i = 0; i < blockq.cnt; i++) {
    rpi_thread_t *t = Q_pop(&blockq);
    if (t->state == BLOCKED) {
        Q_append(&blockq, t);
    } else {
        // output("Thread %d now runnable\n", t->tid);
        Q_append(&runq, t);
    }
  }
}

void print_thread(rpi_thread_t* th) {
  output("Thread %d\n", th->tid);
  output("  next: %d\n", (th->next != NULL) ? th->next->tid : -1);
  output("  svsp: %p\n", th->saved_sp);
  output("  fn:   %p\n", th->fn);
  output("  arg:  %p\n", th->arg);
  output("  annt: %p\n", th->annot);
  output("   stk: %p\n", th->stack);
  output("  done: %d\n", th->finish_time_us);
  output("  stat: %d\n", th->state);
  output("  hash: %d\n", fast_hash(th, sizeof(rpi_thread_t)));
}

void block_threads(void) {
  for (int i = 0; i < runq.cnt; i++) {
    rpi_thread_t *t = Q_pop(&runq);
    if (t->state != BLOCKED) {
        Q_append(&runq, t);
    } else {
        // output("Thread %d now blocked\n", t->tid);
        Q_append(&blockq, t);
    }
  }
}


/***********************************************************************
 * implement the code below.
 */

// stack offsets we expect.
enum {
    R4_OFFSET = 0,
    R5_OFFSET,
    R6_OFFSET,
    R7_OFFSET,
    R8_OFFSET,
    R9_OFFSET,
    R10_OFFSET,
    R11_OFFSET,
    R14_OFFSET = 8,
    LR_OFFSET = 8
};

// return pointer to the current thread.  
rpi_thread_t *rpi_cur_thread(void) {
    return cur_thread;
}

// implement this assembly routine in <asm-checks.S>
uint32_t * push_r0_r12_asm(uint32_t *scratch);
uint32_t * push_r0_r15_asm(uint32_t *scratch);

void print_stupid(void) {
    printk("stupid\n");
}


// called with:
//   - <sp_after_push>: value of the <sp> after the push 
//   - <sp_before_push>: value of the <sp> before the push 
void check_push_order(void) {
    uint32_t scratch_mem[64], 
            // pointer to the middle so doesn't matter if up
            // or down
            *p = &scratch_mem[32];

    uint32_t *ret = push_r0_r12_asm(p);
    // uint32_t *ret = push_r0_r15_asm(p);
    assert(ret < p);
    int target = 0;
    // printk("Vals:\n");
    // for (int i = 19; i < 32; i++) {
    //   printk("r%d: %x ", target, scratch_mem[i]);
    //   target += 1;
    // }
    printk("Vals:\n");
    for (int i = 19; i < 32; i++) {
    // for (int i = 16; i < 32; i++) {
    //   if (i == 29) continue;  // skip r13
      printk("r%d: %x ", target, scratch_mem[i]);
      target += 1;
    }

    // check that regs holds the right values.
    // todo("write the code to check that <ret> holds the expected values.");
    // todo("see caller in <asm-check.S:push_r4_r12_asm>");

    return;

}

// create a new thread.
rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
    rpi_thread_t *t = th_alloc();

    t->fn = code;
    t->arg = arg;
    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     */
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - LR_OFFSET) - 1] = (uint32_t)rpi_init_trampoline;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1] = (uint32_t)code;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R5_OFFSET) - 1] = (uint32_t)arg;
    t->saved_sp = t->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
    t->state = READY;

    // th_trace("Set th=%p fn=%p, arg=%p from code=%p, arg=%p, val=%d\n", t, t->fn, t->arg, code, arg, *(int*)arg);
    th_trace("rpi_fork: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    Q_append(&runq, t);
    return t;
}

// create a new thread.
rpi_thread_t *rpi_fork_rt(void (*code)(void *arg), void *arg, unsigned deadline_us) {
    rpi_thread_t *t = th_alloc();

    t->fn = code;
    t->arg = arg;
    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     */
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - LR_OFFSET) - 1] = (uint32_t)rpi_init_trampoline;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1] = (uint32_t)code;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R5_OFFSET) - 1] = (uint32_t)arg;
    t->saved_sp = t->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
    t->finish_time_us = deadline_us;
    t->state = READY;

    // th_trace("Set th=%p fn=%p, arg=%p from code=%p, arg=%p, val=%d\n", t, t->fn, t->arg, code, arg, *(int*)arg);
    th_trace("rpi_fork: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    Q_append(&runq, t);
    return t;
}

// create a new thread, don't schedule it.
rpi_thread_t *rpi_spawn_nosched(void (*code)(void *arg), void *arg, unsigned deadline_us) {
    rpi_thread_t *t = th_alloc();

    t->fn = code;
    t->arg = arg;
    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     */
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - LR_OFFSET) - 1] = (uint32_t)rpi_init_trampoline;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1] = (uint32_t)code;
    t->stack[THREAD_MAXSTACK - (LR_OFFSET - R5_OFFSET) - 1] = (uint32_t)arg;
    t->saved_sp = t->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
    t->finish_time_us = deadline_us;
    t->state = READY;

    // th_trace("Set th=%p fn=%p, arg=%p from code=%p, arg=%p, val=%d\n", t, t->fn, t->arg, code, arg, *(int*)arg);
    th_trace("rpi_spawn_nosched: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    return t;
}

void rpi_enqueue(rpi_thread_t *t) {
    th_trace("Enqueued: tid=%d, saved_sp=[%p]\n",
            t->tid, t->saved_sp);

    Q_append(&runq, t);
}


// exit current thread.
//   - if no more threads, switch to the scheduler.
//   - otherwise context switch to the new thread.
//     make sure to set cur_thread correctly!
void rpi_exit(int exitcode) {
    unblock_threads();
    // when you switch back to the scheduler thread:
    if (!Q_empty(&runq)) {
        rpi_thread_t *old_thread = cur_thread;
        old_thread->state = STOPPED;
        #if SCHEDULE_BEST_ON_EXIT
          cur_thread = Q_schedule(NULL, &runq, schedule);
        #else
          cur_thread = Q_pop(&runq);
        #endif
        cur_thread->state = RUNNING;
        // cur_thread = Q_find(&runq, rt_cmp);
        // t->saved_sp = t->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
        rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
        return;  // This should not get hit
    }

    th_trace("done running threads, back to scheduler\n");
    rpi_cswitch(&cur_thread->saved_sp, scheduler_thread->saved_sp);
    cur_thread = scheduler_thread;
}

// void push_to_blocked_list(rpi_thread_t *thread) {
//     demand(thread->state == BLOCKED, thread must be in state BLOCKED);
//     runq()
// }

// yield the current thread.
//   - if the runq is empty, return.
//   - otherwise: 
//      * add the current thread to the back 
//        of the runq (Q_append)
//      * context switch to the new thread.
//        make sure to set cur_thread correctly!
void rpi_yield(void) {
    // demand(preemption == 0, yield is not yet compatible with preemption);
    // if (!Q_empty(&runq)) {
    //     rpi_thread_t *old_thread = cur_thread;
    //     old_thread->state = READY;
    //     Q_append(&runq, old_thread);
    //     cur_thread = Q_pop(&runq);
    //     cur_thread->state = RUNNING;
    //     th_trace("switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
    //     rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    // }
    
  if (!Q_empty(&runq)) {
    // printk("in tid %d", cur_thread->tid);
    rpi_thread_t *old_thread = cur_thread;
    // cur_thread = Q_schedule_simple(old_thread, &runq);
    // old_thread->state = READY;
    // cur_thread = Q_schedule(old_thread, &runq, schedule);

    if (old_thread->state != BLOCKED) {
        old_thread->state = READY;
        cur_thread = Q_schedule(old_thread, &runq, schedule);
    } else {
        // demand(false, look what you made me do);
        Q_append(&blockq, old_thread);

        cur_thread = Q_schedule(NULL, &runq, schedule);
    }

    // dev_barrier();
    cur_thread->state = RUNNING;  // This could be the old thread if we rescheduled ourselves (i.e. don't actually switch)
    system_enable_interrupts();
    if (old_thread->saved_sp != cur_thread->saved_sp) {
      th_trace("pr: switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
      rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    }
    // else {
        // th_trace("pr: NOT switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
    // }
    // system_disable_interrupts();
    // th_trace("SHOULD NOT HIT!");
    // system_enable_interrupts();
    // return (uint32_t)cur_thread->saved_sp;
    // rpi_cswitch_preemptive(&old_thread->saved_sp, cur_thread->saved_sp);
  } else {
    system_enable_interrupts();
  }
}

/*
 * starts the thread system.  
 * note: our caller is not a thread!  so you have to 
 * create a fake thread (assign it to scheduler_thread)
 * so that context switching works correctly.   your code
 * should work even if the runq is empty.
 */
void rpi_thread_start(void) {
    th_trace("starting threads!\n");

    // no other runnable thread: return.
    if(Q_empty(&runq))
        goto end;

    // setup scheduler thread block.
    if(!scheduler_thread) {
        scheduler_thread = th_alloc();
        scheduler_thread->saved_sp = scheduler_thread->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
    }

    if (!Q_empty(&runq)) {
        cur_thread = Q_pop(&runq);
        cur_thread->state = RUNNING;
        rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);
    }

end:
    th_trace("done with all threads, returning (%d)\n", scheduler_interrupt_cnt);
}



// helper routine: can call from assembly with r0=sp and it
// will print the stack out.  it then exits.
// call this if you can't figure out what is going on in your
// assembly.
void rpi_print_regs(uint32_t *sp) {
    // use this to check that your offsets are correct.
    printk("cur-thread=%d\n", cur_thread->tid);
    printk("sp=%p\n", sp);

    // stack pointer better be between these.
    printk("stack=%p\n", &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp < &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp >= &cur_thread->stack[0]);
    for(unsigned i = 0; i < 9; i++) {
        unsigned r = i == 8 ? 14 : i + 4;
        printk("sp[%d]=r%d=%x\n", i, r, sp[i]);
    }
    clean_reboot();
}


/* ======================================================================== */
/* =====================   Premptive Threading Start   ==================== */
/* ======================================================================== */

void rpi_thread_start_preemptive(schedule_type schedule_rule) {
    th_trace("starting threads with preemption!\n");
    schedule = schedule_rule;

    // no other runnable thread: return.
    if(Q_empty(&runq))
        goto end;

    // setup scheduler thread block.
    if(!scheduler_thread) {
        scheduler_thread = th_alloc();
        scheduler_thread->saved_sp = scheduler_thread->stack + THREAD_MAXSTACK - (LR_OFFSET - R4_OFFSET) - 1;
    }

  //     if (!Q_empty(&runq)) {
  //   // printk("in tid %d", cur_thread->tid);
  //   rpi_thread_t *old_thread = cur_thread;
  //   // cur_thread = Q_schedule_simple(old_thread, &runq);
  //   cur_thread = Q_schedule(old_thread, &runq, schedule);

  //   th_trace("pr: switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
  //   // dev_barrier();
  //   system_enable_interrupts();
  //   if (old_thread->saved_sp != cur_thread->saved_sp) {
  //     rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
  //   }
  //   // system_disable_interrupts();
  //   // th_trace("SHOULD NOT HIT!");
  //   // system_enable_interrupts();
  //   // return (uint32_t)cur_thread->saved_sp;
  //   // rpi_cswitch_preemptive(&old_thread->saved_sp, cur_thread->saved_sp);
  // } else {
  //   system_enable_interrupts();
  // }

    if (!Q_empty(&runq)) {
        // cur_thread = Q_pop(&runq);
        // cur_thread = Q_find(&runq, rt_cmp);
        cur_thread = Q_schedule(NULL, &runq, schedule);
        // demand(cur_thread->state == READY, scheduled thread must be ready);
        cur_thread->state = RUNNING; // hacky and (obviously) not thread-safe!
        rpi_set_preemption(1);
        rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);
    }

end:
    th_trace("done with all threads, returning (%d)\n", scheduler_interrupt_cnt);
}

void rpi_set_preemption(int val) {
  preemption = val;
  if (val) {
    int_init();
    timer_interrupt_init(0x100);
    // timer_interrupt_init(0xe0);
    system_enable_interrupts();
  } else {
    panic("Disabling preemption not implemented!\n");
  }
}

void interrupt_vector_preemptive(uint32_t pc) {
  // critical_section_start:
  // __asm__ __volatile__("critical_section_start:");
  // asm volatile("critical_section_start:");
  // rpi_print_regs((void *)pc);
  system_disable_interrupts();
  // dev_barrier();
  unsigned pending = GET32(IRQ_basic_pending);

  // if this isn't true, could be a GPU interrupt (as discussed in Broadcom):
  // just return.  [confusing, since we didn't enable!]
  if ((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0) {
    printk("other (DIE)!\n");
    // return pc;
    return;
  };

  
  /*
   * Clear the ARM Timer interrupt - it's the only interrupt we have
   * enabled, so we want don't have to work out which interrupt source
   * caused us to interrupt
   *
   * Q: if we delete?
   */
  PUT32(arm_timer_IRQClear, 1);
  // system_enable_interrupts();
  // printk("Switch\n");
  // printk("pc:%x\n", pc);
  scheduler_interrupt_cnt++;
  if (scheduler_interrupt_cnt % 256 != 0) {
    system_enable_interrupts();
    return;
  }
  // rpi_print_regs((void *)cur_thread->saved_sp);
  // rpi_print_regs((void *)0x104000);

  /*
    TRACE:thread_code:in thread tid=1, with x=0
    TRACE:thread_code:in thread tid=1, with x=0
    TRACE:thread_code:in thread tid=1, with x=0
    TRACE:thread_code:in thread tid=1, with x=0
    TRACE:thread_code:in thread tid=2, with x=1
    cur-thread=2
    sp=0x104000
    stack=0x104040
    sp[0]=r4=0x9900
    sp[1]=r5=0x20000113
    sp[2]=r6=0x5aa6
    sp[3]=r7=0xa
    sp[4]=r8=0xa97c
    sp[5]=r9=0x5aa5
    sp[6]=r10=0x31
    sp[7]=r11=0x8098
    sp[8]=r14=0x102020
  */

  /* Handle context-switch logic here!!! */
//   output("intr\n");

  // Preemptive yield
  // printk("first in tid %d", cur_thread->tid);
  if (!Q_empty(&runq)) {
    // printk("in tid %d", cur_thread->tid);
    rpi_thread_t *old_thread = cur_thread;
    // cur_thread = Q_schedule_simple(old_thread, &runq);
    if (old_thread->state != BLOCKED) {
        old_thread->state = READY;
        cur_thread = Q_schedule(old_thread, &runq, schedule);
    } else {
        // demand(false, look what you made me do);
        Q_append(&blockq, old_thread);

        cur_thread = Q_schedule(NULL, &runq, schedule);
    }
    // output("Scheduled tid=%d\n", cur_thread->tid);
    // demand(cur_thread->state == READY, scheduled thread must be ready);
    cur_thread->state = READY; // hacky and (obviously) not thread-safe!

    // dev_barrier();
    cur_thread->state = RUNNING;  // This could be the old thread if we rescheduled ourselves (i.e. don't actually switch)
    system_enable_interrupts();
    if (old_thread->saved_sp != cur_thread->saved_sp) {
      th_trace("pr: switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
      rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    }
    // else {
        // th_trace("pr: NOT switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
    // }
    // system_disable_interrupts();
    // th_trace("SHOULD NOT HIT!");
    // system_enable_interrupts();
    // return (uint32_t)cur_thread->saved_sp;
    // rpi_cswitch_preemptive(&old_thread->saved_sp, cur_thread->saved_sp);
  } else {
    system_enable_interrupts();
  }
  // critical_section_end:
  // asm volatile("critical_section_end:");

  // rpi_yield();

  // increment the counter for <pc>.
  // gprof_inc(pc);

  // this doesn't need to stay here.
  // static unsigned last_clk = 0;
  // unsigned clk = timer_get_usec();
  // period = last_clk ? clk - last_clk : 0;
  // last_clk = clk;

  // Q: if we put a print statement?
  // th_trace("Mysteriously HIT!");
  // system_enable_interrupts();
  // dev_barrier();
  // return pc;
}

/* ======================================================================== */
/* ======================   Premptive Threading End   ===================== */
/* ======================================================================== */
