#include "rpi.h"
#include "gpio.h"
#include "servo.h"
#include "gpio-int.h"

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
  gpio_set_on(pin);
  delay_us(500 + angle * 10);
  gpio_set_off(pin);
  delay_us(20000 - 500 - angle * 10);  
}

