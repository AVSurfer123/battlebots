#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__
#include "armv6-debug.h"
#include "rpi.h"

// all your code should go here.  implementation of the debug interface.

// example of how to define get and set for status registers
coproc_mk(dscr, p14, 0, c0, c1, 0)
coproc_mk(dfsr, p15, 0, c5, c0, 0)
coproc_mk(ifsr, p15, 0, c5, c0, 1)
coproc_mk(far, p15, 0, c6, c0, 0)
coproc_mk(ifar, p15, 0, c6, c0, 2)
coproc_mk(bvr0, p14, 0, c0, c0, 4)
coproc_mk(bcr0, p14, 0, c0, c0, 5)
coproc_mk(wvr0, p14, 0, c0, c0, 6)
coproc_mk(wcr0, p14, 0, c0, c0, 7)
coproc_mk(wfar, p14, 0, c0, c6, 0)


// you'll need to define these and a bunch of other routines.
// static inline uint32_t cp15_dfsr_get(void);
// static inline uint32_t cp15_ifar_get(void);
// static inline uint32_t cp15_ifsr_get(void);
// static inline uint32_t cp14_dscr_get(void);
// static inline void cp14_wcr0_set(uint32_t r);

// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
static inline int cp14_is_enabled(void) {
    uint32_t status = cp14_dscr_get();
    return bit_get(status, 14) == 0 && bit_get(status, 15) == 1;   
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return?
    if(cp14_is_enabled())
        panic("already enabled\n");

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t status = cp14_dscr_get();
    status = bit_set(status, 15);
    status = bit_clr(status, 14);
    cp14_dscr_set(status);

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    uint32_t status = cp14_dscr_get();
    status = bit_clr(status, 15);
    cp14_dscr_set(status);

    assert(!cp14_is_enabled());
}


static inline int cp14_bcr0_is_enabled(void) {
    return bit_get(cp14_bcr0_get(), 0);
}
static inline void cp14_bcr0_enable(void) {
    uint32_t val = cp14_bcr0_get();
    val = bit_set(val, 0);
    cp14_bcr0_set(val);
}
static inline void cp14_bcr0_disable(void) {
    uint32_t val = cp14_bcr0_get();
    val = bit_clr(val, 0);
    cp14_bcr0_set(val);
}

// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
    uint32_t status = cp15_ifsr_get();
    if (bits_get(status, 0, 3) != 0b10) {
        return 0;
    }
    status = cp14_dscr_get();
    return bits_get(status, 2, 5) == 1;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
    uint32_t status = cp15_dfsr_get();
    if (bits_get(status, 0, 3) != 0b10) {
        return 0;
    }
    status = cp14_dscr_get();
    return bits_get(status, 2, 5) == 0b10;
}

static inline int cp14_wcr0_is_enabled(void) {
    return bit_get(cp14_wcr0_get(), 0);
}
static inline void cp14_wcr0_enable(void) {
    uint32_t val = cp14_wcr0_get();
    val = bit_set(val, 0);
    cp14_wcr0_set(val);
}
static inline void cp14_wcr0_disable(void) {
    uint32_t val = cp14_wcr0_get();
    val = bit_clr(val, 0);
    cp14_wcr0_set(val);
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {
    uint32_t addr = cp14_wfar_get();
    return addr - 8;
}

static inline uint32_t watchpt_fault_addr(void) {
    uint32_t addr = cp15_far_get();
    return addr;
}
    
#endif
