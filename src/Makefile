
# all the tests
# PROGS := $(wildcard tests/[0123]-*.c)
PROGS := 1-blink.c
PROGS := 3-one-way-ack-Nbytes.c
PROGS := 1-rising-edge.c

COMMON_SRC := test-interrupts.c

STAFF_OBJS := ../libpi/staff-objs/new-spi.o

LIBS = ./libgcc.a

# uncomment if you want it to automatically run.
RUN=0

# define this if you need to give the device for your pi
BOOTLOADER = my-install

# GREP_STR := 'HASH:\|ERROR:\|PANIC:\|SUCCESS:\|NRF:'
EXCLUDE ?= grep -v simple_boot
include ../libpi/mk/Makefile.template-v2
