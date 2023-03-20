#include "rpi.h"
#include "pwm.h"

#define PLLC_CNTL 0x5c
#define PLLC_DIV 0x60
#define PLLC_STAT 0x64
#define GPIO_BASE 0x20200000

#define CLK_BASE_ADDR CLOCK_MANAGER // Base address for clock module
#define PWM_CLOCK_ID 0              // ID of the PWM clock (BCM2835 has only one PWM clock)
#define PWM_CLOCK_DIVIDER 16        // Set the PWM clock divider to 16

void pwm_config_clock(uint32_t divisor)
{
    assert(divisor < 0xfff);

    // Stop PWM clock
    PUT32(PWM_CLK_CTL, PWM_PASSWD | CLK_CTL_STOP);
    delay_ms(110);

    // Wait for the clock to be not busy
    while ((GET32(PWM_CLK_CTL) & CLK_CTL_BUSY))
        delay_ms(1);

    // set the clock divider
    PUT32(PWM_CLK_DIV, PWM_PASSWD | (divisor << 12));

    // enable PWM clock
    PUT32(PWM_CLK_CTL, PWM_PASSWD | CLK_CTL_ENAB);
}

// this function initalizes a given pin to pwm then retruns which PWM it is (0 or 1)
// Broadcom p 102
// gpio12 - alt0 - PWM0
// gpio13 - alt0 - PWM1
// gpio18 - alt5 - PWM0
// gpio19 - alt5 - PWM1
// gpio40 - alt0 - PWM0
// gpio41 - alt0 - PWM1
// gpio45 - alt0 - PWM1
// there are two more pins on p140 but not on p102
uint8_t pwm_init_gpio(uint32_t pin)
{
    switch (pin)
    {
    case 12:
    case 40:
        gpio_set_function(pin, GPIO_FUNC_ALT0);
        return 1;
    case 13:
    case 41:
    case 45:
        gpio_set_function(pin, GPIO_FUNC_ALT0);
        trace("pin %d: set for PMW1 (GPIO_FUNC_ALT0)\n", pin);
        return 2;
    case 18:
        gpio_set_function(pin, GPIO_FUNC_ALT5);
        return 1;
    case 19:
        gpio_set_function(pin, GPIO_FUNC_ALT5);
        return 2;
    default:
        panic("pwm initization: pin %d does not have PWM\n", pin);
        break;
    }
}

void pwm_init(uint8_t pin, uint32_t freq, uint32_t duty_cycle)
{
    uint32_t resolution = 2048;
    uint32_t data = resolution * duty_cycle / 100;

    // initalize pin for PWM
    uint8_t pwm_channel = pwm_init_gpio(pin);

    // configure pwm clock
    pwm_config_clock(16);

    // configure pwm registers
    // disable pwm
    PUT32(PWM_CTL, DISABLE_PWM_CTL);

    if(pwm_channel == 1){
        // set pwm data and range
        PUT32(PWM_RNG1, resolution); // set PWM range to 1024
        PUT32(PWM_DAT1, data); // set PWM duty cycle

        // enable PWM - read write modify
        PUT32(PWM_CTL, GET32(PWM_CTL) | ENABLE_PWM_CTL_1);
    }
    if(pwm_channel == 2){
        // set pwm data and range
        PUT32(PWM_RNG2, resolution); // set PWM range to 1024
        PUT32(PWM_DAT2, data); // set PWM duty cycle

        // enable PWM - read write modify
        PUT32(PWM_CTL, GET32(PWM_CTL) | ENABLE_PWM_CTL_2);
    }
}