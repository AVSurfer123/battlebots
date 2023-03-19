#ifndef __PWM_H__
#define __PWM_H__


// =========================== pwm clock defs ===========================
enum{
    CLOCK_MANAGER = 0x20101000,
    PLLC_CTL = (CLOCK_MANAGER),
    PLLC_DIV = (CLOCK_MANAGER + 0x4),
    PLLC_STAT = (CLOCK_MANAGER + 0x8),
    PLLD_CTL = (CLOCK_MANAGER + 0xC),
    PWM_CLOCK = (CLOCK_MANAGER + 0xA0), 
    PWM_CLK_CTL = (PWM_CLOCK), 
    PWM_CLK_DIV = (PWM_CLOCK + 0x4)

};
#define PWM_PASSWD      0x5A000000 //idek chatgpt told me to
#define CLK_CTL_STOP    0x1
#define CLK_CTL_BUSY    0x8
#define CLK_CTL_ENAB    0x11

// =========================== pwm reg defs ===========================
// register ranges - broadcom p141
enum {
    PWM_BASE = 0x2020C000,
    PWM_CTL = PWM_BASE,  // PWM control
    PWM_STA  = (PWM_BASE + 0x4), // PWM satsus
    PWM_DMAC  = (PWM_BASE + 0x8), // PWM DMA config
    PWM_RNG1  = (PWM_BASE + 0x10), // PWM channel 1 range
    PWM_DAT1 = (PWM_BASE + 0x14), // PWM channel 1 data
    PWM_FIF1 = (PWM_BASE + 0x18), // PWM FIFO input
    PWM_RNG2 = (PWM_BASE + 0x20), // PWM channel 2 range
    PWM_DAT2 = (PWM_BASE + 0x24) // PWM channel 2 data
};
// status register
    // CTL reg
    // bit 0 - PWEN1 (channel is enabled/disabled)                      1
    // bit 1 - MODE1 (PWM/serialiser moed)                              0
    // bit 2 - RPTL1 (repeal last data)                                 0
    // bit 3 - SBIT1 (silence bit - state of output no tranmission)     0
    // bit 4 - POLA1 (polarity/invert bit)                              0
    // bit 5 - USEF1 (use data reg or fifo)                              0
    // bit 6 - CLRF1 (clear FIFO)                                        0
    // bit 7 - MSEN1 (PWM/MS algortihm)                                 0
    // bit 8 - PWEN2 (channel is enabled/disabled)                      1
    // bit 9 - MODE2 (PWM/serialiser moed)                              0
    // bit 10 - RPTL2 (repeal last data)                                0
    // bit 11 - SBIT2 (silence bit - state of output no tranmission)    0
    // bit 12 - POLA2 (polarity/invert bit)                             0
    // bit 13 - USEF (use data reg or fifo)                             0
    // bit 14 - reserved
    // bit 15 - MSEN2 (PWM/MS algortihm)                                0
    // bits 16-31 - reserverd
#define ENABLE_PWM_CTL_1 0x1
#define ENABLE_PWM_CTL_2 0x100
#define DISABLE_PWM_CTL 0x0
#define PWM_DMAC_VAL    ((1 <<31) | (15 << 8) | 15)


// =========================== functions ===========================
void pwm_clock_config(uint32_t freq);

uint8_t pwm_init_gpio(uint32_t pin);

void pwm_init(uint8_t pin, uint32_t freq, uint32_t duty_cycle);

#endif