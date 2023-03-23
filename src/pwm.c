#include "rpi.h"
#include "pwm.h"

#define PRINT           1

uint32_t clock_configed = 0;
void pwm_config_clock(uint32_t divisor)
{
    if(clock_configed)
        return;

    if(PRINT) trace("config pwm clock\n");
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

    clock_configed = 1;
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
uint8_t pwm_gpio(uint32_t pin, uint32_t init)
{
    switch (pin)
    {
    case 12:
    case 40:
        if(init){
            gpio_set_function(pin, GPIO_FUNC_ALT0);
            if(PRINT) trace("pin %d: set for PMW0 (GPIO_FUNC_ALT0)\n", pin);
        }
        return 1;
    case 13:
    case 41:
    case 45:
        if(init){
            gpio_set_function(pin, GPIO_FUNC_ALT0);
            if(PRINT) trace("pin %d: set for PMW1 (GPIO_FUNC_ALT0)\n", pin);
        }
        return 2;
    case 18:
        if(init){
            gpio_set_function(pin, GPIO_FUNC_ALT5);
            if(PRINT) trace("pin %d: set for PMW0 (GPIO_FUNC_ALT5)\n", pin);
        }
        return 1;
    case 19:
        if(init){
            gpio_set_function(pin, GPIO_FUNC_ALT5);
            if(PRINT) trace("pin %d: set for PMW1 (GPIO_FUNC_ALT5)\n", pin);
        }
        return 2;
    default:
        panic("pwm initization: pin %d does not have PWM\n", pin);
        break;
    }
}

void pwm_set(uint8_t pin, uint32_t duty_cycle)
{
    if(PRINT) trace("pwm set on %d with duty_cycle=%d\n", pin, duty_cycle);
    uint32_t resolution = 3000;
    uint32_t data = resolution * duty_cycle / 100;

    // initalize pin for PWM
    uint8_t pwm_channel = pwm_gpio(pin, 0);

    // configure pwm registers
    // disable pwm
    uint32_t og_control = GET32(PWM_CTL);
    PUT32(PWM_CTL, DISABLE_PWM_CTL);

    if (pwm_channel == 1)
    {
        // set pwm data and range
        PUT32(PWM_RNG1, resolution);
        PUT32(PWM_DAT1, data);       // set PWM duty cycle

        // enable PWM - read write modify
        PUT32(PWM_CTL, og_control | ENABLE_PWM_CTL_1);
    }
    if (pwm_channel == 2)
    {
        // set pwm data and range
        PUT32(PWM_RNG2, resolution);
        PUT32(PWM_DAT2, data);       // set PWM duty cycle

        // enable PWM - read write modify
        PUT32(PWM_CTL, og_control | ENABLE_PWM_CTL_2);
    }
}

void pwm_init(uint8_t pin, uint32_t duty_cycle)
{
    if(PRINT) trace("pwm init on %d with duty_cycle=%d\n", pin, duty_cycle);
    uint32_t resolution = 3000;
    uint32_t data = resolution * duty_cycle / 100;

    // initalize pin for PWM
    uint8_t pwm_channel = pwm_gpio(pin, 1);

    // configure pwm registers
    // disable pwm
    uint32_t og_control = GET32(PWM_CTL);
    PUT32(PWM_CTL, DISABLE_PWM_CTL);

    // configure pwm clock
    pwm_config_clock(128);

    if (pwm_channel == 1)
    {
        // set pwm data and range
        PUT32(PWM_RNG1, resolution); // set PWM range
        delay_ms(50);
        PUT32(PWM_DAT1, data);       // set PWM duty cycle
        delay_ms(50);

        // enable PWM - read write modify
        PUT32(PWM_CTL, og_control | ENABLE_PWM_CTL_1);
    }
    if (pwm_channel == 2)
    {
        // set pwm data and range
        PUT32(PWM_RNG2, resolution); // set PWM range
        delay_ms(50);
        PUT32(PWM_DAT2, data);       // set PWM duty cycle
        delay_ms(50);

        // enable PWM - read write modify
        PUT32(PWM_CTL, og_control | ENABLE_PWM_CTL_2);
    }
    delay_ms(50);
    trace("pwm status: %x\n", GET32(PWM_STA));
}

void pwm_stop(){
    PUT32(PWM_CTL, DISABLE_PWM_CTL);
}