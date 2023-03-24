#include "rpi.h"
#include "pwm.h"

#define    CONTIUOS_PWM_PIN      13
#define    SERVO_PWM_PIN     12

void notmain(void) { 
    pwm_init(SERVO_PWM_PIN, 8);
    // pwm_init(CONTIUOS_PWM_PIN, 1);

    delay_ms(1000);

    for(int i=0; i<11; i++){
        pwm_set(SERVO_PWM_PIN, 13-i*1);
        delay_ms(1000);
    }
    // pwm_set(SERVO_PWM_PIN, 12);
    // delay_ms(2000);


    pwm_stop();
    gpio_set_output(SERVO_PWM_PIN);
    gpio_set_off(SERVO_PWM_PIN);
}

