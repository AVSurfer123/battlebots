@ hacked code for testing vector base code and testing the
@ difference between the position-independent "generic" method
@ of last lab (that used the ldr instruction) with the faster
@ method of a relative branch (so: no memory load, no cache miss,
@ also, the ldr will commonly = a misprediction).
#include "rpi-asm.h"

.align 5; 
.globl _interrupt_table
_interrupt_table:
    b unhandled_reset
    b unhandled_undefined_instruction
    b unhandled_swi
    b unhandled_prefetch_abort
    b unhandled_data_abort
    b unhandled_reset // unused
    b unhandled_interrupt
    b unhandled_fiq
