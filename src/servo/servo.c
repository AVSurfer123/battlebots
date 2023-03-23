#include "rpi.h"
#include "gpio.h"
#include "servo.h"

#define PWM_RES 9.5 // us / deg
#define PWM_RANGE 180 * PWM_RES
#define PWM_MIN 1500 - PWM_RANGE / 2

void init_servo(int pin) {
  gpio_set_output(pin);
  gpio_set_off(pin);
}

void init_input_servo(int pin, int trigger) {
  gpio_set_output(pin);
  gpio_set_input(trigger);

  gpio_set_off(pin);
}

// TODO: reference servo datasheet
void set_servo(int pin, int angle) {
  // loop should run at 50 Hz
  gpio_set_on(pin);
  int pulse = PWM_MIN + angle * PWM_RES;
  delay_us(pulse);
  gpio_set_off(pin);
  delay_us(20000 - pulse);  
}
