# to start with use our objects.
# for checkoff this should be 0
USE_STAFF = 0

PART ?= 1

# the different tests
# you can also just assign PROGS manually.
PROGS = $(wildcard ./test-*.c)
# uncomment if you want it to automatically run.
# RUN=1


# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = pi-install

##################################################################
# shouldn't have to touch below.



# use our kmalloc
DIR := $(CS140E_2023_PATH)/libpi
STAFF_OBJS := $(DIR)/staff-objs/kmalloc.o

COMMON_SRC += thread-asm.S 
COMMON_SRC += interrupts-asm.S 
COMMON_SRC += rpi-thread.c
COMMON_SRC += timer-interrupt.c
COMMON_SRC += interrupts-c.c

CAN_EMIT = 0

# there must be a better way to do this.
ifeq ($(MAKECMDGOALS),checkoff)
    PROGS = $(wildcard ./[345678]-test*.c)
    USE_STAFF = 0
endif

GREP_STR := 'TRACE:\|ERROR:\|PANIC:'

include $(CS140E_2023_PATH)/libpi/mk/Makefile.template

checkoff: check
	@echo "--------------------------------------------------------";
	@echo "Environment:";
	@echo "   STAFF_OBJS [should only have kmalloc] =<$(STAFF_OBJS)";
	@echo "   COMMON_SRC [should have both .o] =<$(COMMON_SRC)>";
	@echo -n "   total tests=";
	@ls $(PROGS:.c=.out) | wc -l
	@echo -n "   checksum of cksum: ";
	@cksum $(PROGS:.c=.out) | sort -n | cksum

.PHONEY: checkoff
