#include "rpi.h"
#include "pwm.h"

#define    PWM_PIN      12
#define    GPIO_PIN     13

void notmain(void) { 
    gpio_set_output(GPIO_PIN);
    gpio_set_on(GPIO_PIN);

    pwm_init(PWM_PIN, 7, 50);

    for(int i=0; i<100; i++){
        gpio_set_on(GPIO_PIN);
        delay_ms(50);
        gpio_set_off(GPIO_PIN);
        delay_ms(50);
    }

}

