PROG_SRC = read_joystick.c send_joystick_to_pi.c
SRC = 

CFLAGS += -pthread

LU = ../../libunix

include ../../libunix/mk/Makefile.unix

trace:
	./my-install --trace-control hello.bin

trace-all:
	./my-install --trace-all hello.bin

.PHONEY: trace trace-all
