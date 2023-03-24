/*
 * implement these.  ordered in difficulty.
 */
#include "rpi-asm.h"

@ return the current stack pointer.
MK_FN(rpi_get_sp)
    asm_todo("implement this")
    bx lr

MK_FN(rpi_cswitch)
    @ also push and save spsr
    push {r4-r11, lr}
    @ bl check_push_order
    @ bl rpi_init_trampoline
    str sp, [r0]  @ Figure out where to put this
    mov sp, r1
    pop {r4-r11, lr}
    bx lr

MK_FN(rpi_cswitch_preemptive)
    @ stmia sp, {sp, lr}^
    @ push {r0-r12}

    @ Switch into cpsr to get banked regs
    mrs r2, cpsr
    msr cpsr_c, #SUPER_MODE
    @ mov r0, r13     @ get banked sp
    @ mov r1, r14     @ get banked lr {Subtract 4 from LR to account for interrupt?}
    @ what to do with them????
    push {r13-r14}
    msr cpsr_c, r2    @ Go back into IRQ

    @ bl check_push_order
    str sp, [r0]      @ store current sp in returned pointer
    mov sp, r1        @ load new sp

    @ pop {r13-r14}
    ldmia sp, {r13-r14}^    @ ?????

    pop {r0-r12}
    @ ldmia sp, {sp, lr}^
    bx lr

MK_FN(push_r0_r12_asm)
    @ save caller state
    push {r0-r12}   @ save the registers we will trash.
    mov r1, sp      @ save sp to scratch register


    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @ do the push with fixed values.

    @ move the passed in ptr (in r0) to sp
    mov sp, r0
    push {r0-r12}   @ push to given memory block.
    mov r0, sp      @ return where sp ended up.

    @ done
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    @ restore caller state
    mov sp, r1      @ restore sp
    pop {r0-r12}    @ restore trashed regs. Should this be r4- or r0- ?
    bx lr
    asm_todo(implement check_push_order_asm)

MK_FN(push_r0_r15_asm)
    @ save caller state
    push {r0-r12,r15}   @ save the registers we will trash.
    mov r1, sp      @ save sp to scratch register


    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @ do the push with fixed values.

    @ move the passed in ptr (in r0) to sp
    mov sp, r0
    push {r0-r12,r15}   @ push to given memory block.
    mov r0, sp      @ return where sp ended up.

    @ done
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    @ restore caller state
    mov sp, r1      @ restore sp
    pop {r0-r12,r15}    @ restore trashed regs
    bx lr
    asm_todo(implement check_push_order_asm)

@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
MK_FN(rpi_init_trampoline)
    @ ldr lr, =rpi_init_trampoline // Unsure about this
    mov r0, r5  // Load arg into r0
    mov r1, r4  // Load fn into r1
    blx r1     // Branch and link to fn (which is in r1)
    bl rpi_exit
    asm_todo (death)  // died!
