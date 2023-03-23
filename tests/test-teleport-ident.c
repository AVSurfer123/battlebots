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
#include "nrf-test.h"
#include "fast-hash32.h"
#include "mbox.h"

static unsigned thread_count, thread_sum;

static int finished = 0;
static uint32_t serialno;
#define NBYTES 32
static int pulses = 1;

static struct lock test_lock;
static struct semaphore barrier;
static void send_thread(void *arg);
// static nrf_t *s = NULL;
// static nrf_t *c = NULL;
enum { ntrial = 1000, timeout_usec = 100000000, nbytes = NBYTES };
static int tina_addr = 0xe5e5e5;
static int luca_addr = 0xd5d5d5;

static void clone_code(void *arg) {
  trace("cloned!\n");
  rpi_exit(0);
}

nrf_t *s = NULL;
nrf_t *c = NULL;

void swap_addrs() {
  int temp = tina_addr;
  tina_addr = luca_addr;
  luca_addr = temp;
}

// trivial first thread: does not block, explicitly calls exit.
static void receive_thread(void *arg) {
  // if (serialno == 3193211922) {
    // sema_down(&barrier);
    // output("passed receive sema\n");
    // delay_ms(2000);
  // }
  // lock_acquire(&test_lock);
  unsigned *x = arg;
  // nrf_t *s = server_mk_noack(luca_addr, nbytes);

  // check tid
  unsigned tid = rpi_cur_thread()->tid;
  trace("acquired lock in thread tid=%d, with x=%d\n", tid, *x);
  // demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
  //        *x + 1);

  thread_sum += *x;

  unsigned npackets = 0, ntimeout = 0;
  uint32_t exp = 0, got = 0;

  // char buf[100];
  // memset(buf, 0, sizeof(buf));
  rpi_thread_t *t = th_alloc();
  // char buf[100];
  // memset(buf, 0, sizeof(buf));

  // for(unsigned i = 0; i < ntrial; i += nbytes) {
  //     int ret = nrf_read_exact_timeout(s, buf + i, nbytes, timeout_usec);
  //     output("buf: %s, i: %d\n", buf, i);
  //     if (*(buf + i) == '\0')
  //       break;
  // }

  uint32_t stack_offset[NBYTES / sizeof (uint32_t)];
  int ret = nrf_read_exact_timeout(s, stack_offset, nbytes, timeout_usec);
  t->saved_sp = t->stack + stack_offset[0];

  char *codebuf = kmalloc(4096);
  for(unsigned i = 0; i < 4096; i += nbytes) {
      int ret = nrf_read_exact_timeout(s, codebuf + i, nbytes, timeout_usec);
  }
  if (fast_hash32(codebuf, 512) != fast_hash32(clone_code, 512)) {
    output("Received code: %d\n", fast_hash32(codebuf, 512));
    output("Compare to: %d\n", fast_hash32(clone_code, 512));
    memcpy(codebuf, clone_code, 4096);
  }

  unsigned deadline_us = 100000000;
  int *arg2 = kmalloc(sizeof *arg2);
  // rpi_thread_t *guineapig = rpi_spawn_nosched((void(*)(void*))clone_code, arg2, deadline_us);
  // rpi_thread_t *guineapig = th_alloc();
  // rpi_thread_t *guineapig2 = rpi_spawn_nosched((void(*)(void*))clone_code, arg2, deadline_us);
  // rpi_thread_t *guineapig3 = rpi_spawn_nosched((void(*)(void*))clone_code, arg2, deadline_us);
  // rpi_thread_t *guineapig = rpi_spawn_nosched(clone_code, arg2, deadline_us);
  // output("origpig:\n");
  // print_thread(guineapig);
  // output("guineapig2:\n");
  // print_thread(guineapig2);
  // output("guineapig3:\n");
  // print_thread(guineapig3);

  for(unsigned i = 0; i < sizeof(rpi_thread_t); i += nbytes) {
      int ret = nrf_read_exact_timeout(s, ((char*)t) + i, nbytes, timeout_usec);
  }
  // output("got guineapig: %d\n", fast_hash(t, sizeof(rpi_thread_t)));
  print_thread(t);
  // memcpy(guineapig->stack, t->stack, sizeof(rpi_thread_t));
  // for (int i = 0; i < (THREAD_MAXSTACK / 512); i++) {
  //   output("g stack[%d-%d]: %d\n", i * 512, (i + 1) * 512, fast_hash(((char*)guineapig) + i * 512, 512));
  //   output("t stack[%d-%d]: %d\n", i * 512, (i + 1) * 512, fast_hash(((char*)t) + i * 512, 512));
  // }
  // memcpy(t->stack, guineapig->stack, THREAD_MAXSTACK * 4);
  t->saved_sp = (uint32_t*)(((char*) t->stack) + stack_offset[0]);
  output("changedpig:\n");
  print_thread(t);
  rpi_enqueue(t);
  // unsigned deadline_us = timer_get_usec() + 100000;
  // int *arg2 = kmalloc(sizeof *arg2);
  // rpi_thread_t *guineapig = rpi_spawn_nosched(clone_code, arg2, deadline_us);
  // rpi_enqueue(guineapig);

  trace("trial: total successfully sent %d ack'd packets lost [%d]\n",
      npackets, ntimeout);

  trace("trial: total successfully sent %d ack'd packets lost [%d]\n",
      npackets, ntimeout);

  finished += 1;
  trace("Finished thread tid=%d!\n", tid);

  if (pulses > 0) {
    pulses -= 1;
    swap_addrs();
    delay_ms(1500);
    send_thread(arg);
  }

  // if (serialno != 3193211922)
    // sema_up(&barrier);
  rpi_exit(0);
}

// trivial first thread: does not block, explicitly calls exit.
static void send_thread(void *arg) {
  // if (serialno != 3193211922) {
  //   sema_down(&barrier);
  //   output("passed send sema\n");
  //   delay_ms(3000);
  // }
  // lock_acquire(&test_lock);
  unsigned *x = arg;
  // nrf_t *c = client_mk_noack(luca_addr, nbytes);

  // check tid
  unsigned tid = rpi_cur_thread()->tid;
  trace("acquired lock in thread tid=%d, with x=%d\n", tid, *x);
  // demand(rpi_cur_thread()->tid == *x + 1, "expected %d, have %d\n", tid,
  //        *x + 1);

  thread_sum += *x;

  unsigned npackets = 0, ntimeout = 0;
  uint32_t exp = 0, got = 0;

  // char msg[] = "hello world this is data being sent by luca pistor";
  // for(unsigned i = 0; i < sizeof(msg) + 1; i += nbytes) {
  //   trace("sent %d ack'd packets [timeouts=%d]\n", 
  //                 npackets, ntimeout);

  //     trace("sending %c\n", *(msg + i));
  //     int ret = nrf_send_noack(c, luca_addr, msg + i, nbytes);
  // }

  unsigned deadline_us = 100000000;
  int *arg2 = kmalloc(sizeof *arg2);
  rpi_thread_t *guineapig = rpi_spawn_nosched(clone_code, arg2, deadline_us);
  // rpi_thread_t *sendpig2 = rpi_spawn_nosched(clone_code, arg2, deadline_us);
  // rpi_thread_t *sendpig3 = rpi_spawn_nosched(clone_code, arg2, deadline_us);
  // output("sendpig:\n");
  // print_thread(guineapig);
  // output("sendpig2:\n");
  // print_thread(sendpig2);
  // output("sendpig3:\n");
  // print_thread(sendpig3);
  // output("sending guineapig: %d\n", fast_hash(guineapig, sizeof(rpi_thread_t)));
  // for (int i = 0; i < 20; i++) {
  //   output("sending guineapig[%d-%d]: %d\n", i * 100, (i + 1) * 100, fast_hash(guineapig + i * 100, 100));
  // }

  uint32_t stack_offset[NBYTES / sizeof (uint32_t)];
  stack_offset[0] = (char*)guineapig->saved_sp - (char*)guineapig->stack;
  output("Sending stack offset: %d\n", stack_offset[0]);
  int ret = nrf_send_noack(c, luca_addr, stack_offset, nbytes);


  output("Sending code: %d\n", fast_hash32(clone_code, 512));
  print_thread(guineapig);
  for(unsigned i = 0; i < 4096; i += nbytes) {
      // trace("sending %c\n", *(msg + i));
      delay_ms(10);
      int ret = nrf_send_noack(c, luca_addr, ((char*)clone_code) + i, nbytes);
  }

  output("Sending thread:\n");
  print_thread(guineapig);
  for(unsigned i = 0; i < sizeof(rpi_thread_t) + 1; i += nbytes) {
  // for(unsigned i = 0; i < sizeof(msg) + 1; i += nbytes) {
    // trace("sent %d ack'd packets [timeouts=%d]\n", 
    //               npackets, ntimeout);
    // if (i == nbytes * 200) trace("Sent %d / %d\n", i, sizeof(rpi_thread_t));
    if (i % 4000 == 0) trace("Sent %d / %d\n", i, sizeof(rpi_thread_t));

      // trace("sending %c\n", *(msg + i));
      delay_ms(10);
      int ret = nrf_send_noack(c, luca_addr, ((char*)guineapig) + i, nbytes);
  }

  trace("trial: total successfully sent %d ack'd packets lost [%d]\n",
      npackets, ntimeout);

  finished += 1;
  trace("Finished thread tid=%d!\n", tid);

  if (pulses > 0) {
    pulses -= 1;
    swap_addrs();
    receive_thread(arg);
  }

  // if (serialno == 3193211922)
  //   sema_up(&barrier);
  rpi_exit(0);
}

void notmain() {
  test_init();
  sema_init(&barrier, 0);

  // change this to increase the number of threads.
  trace("about to test teleporting a thread\n");

  serialno = serialno_trunc();
  trace("Serial number: %u\n", serialno);

  unsigned deadline_us = timer_get_usec() + 100000;
  int *x = kmalloc(sizeof *x);

  thread_sum = thread_count = 0;
  if (serialno == 3193211922) {
    c = client_mk_noack(tina_addr, nbytes);
    s = server_mk_noack(luca_addr, nbytes);
    output("SPAWNING AS RECEIVER!\n");
    rpi_fork_rt(receive_thread, x, deadline_us);
  } else {
    c = client_mk_noack(luca_addr, nbytes);
    s = server_mk_noack(tina_addr, nbytes);
    output("SPAWNING AS SENDER!\n");
    rpi_fork_rt(send_thread, x, deadline_us);
  }

  // if (serialno != 3193211922) {
  //   c = client_mk_noack(tina_addr, nbytes);
  //   output("SPAWNING AS SENDER!\n");
  //   rpi_fork_rt(send_thread, x, deadline_us);
  // } else {
  //   s = server_mk_noack(tina_addr, nbytes);
  //   output("SPAWNING AS RECEIVER!\n");
  //   rpi_fork_rt(receive_thread, x, deadline_us);
  // }
  // lock_init(&test_lock);

  rpi_thread_start_preemptive(SCHEDULE_RTOS);

  // emit all the stats.
  // nrf_stat_print(c, "luca: done with one-way test");
  // no more threads: check.
  trace("count = %d, sum=%d, finished=%d\n", thread_count, thread_sum, finished);
  // assert(thread_sum == sum);
  trace("SUCCESS!\n");
}

