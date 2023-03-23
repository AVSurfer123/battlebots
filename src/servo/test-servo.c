#include "rpi.h"
#include "servo.h"


int pin = 19;
int trigger = 26;


void interrupt_vector(unsigned pc) {

  dev_barrier();

  if(gpio_event_detected(trigger)){
    gpio_event_clear(trigger);
    set_servo(pin, 45);
    delay_ms(100);
    set_servo(pin, 90);
    delay_ms(100);
    set_servo(pin, 135);
    delay_ms(100);
    set_servo(pin, 90);
    delay_ms(100);
  }

  // add specific code here

  dev_barrier();
  return;
}


void notmain() {
  printk("beginning servo test\n");
  // unimplemented();

  init_servo(pin);
  set_servo(pin, 0);
  delay_ms(1000);
  set_servo(pin, 90);
  delay_ms(1000);
  set_servo(pin, 180);
  delay_ms(1000);
  set_servo(pin, 90);
  delay_ms(1000);
  set_servo(pin, 0);

  printk("about to install handlers\n");
  int_init();

  printk("setting up gpio rising edge interrupt on pin %d\n", trigger);
  gpio_set_input(trigger);
  gpio_int_rising_edge(trigger);
  gpio_int_falling_edge(trigger);
  gpio_event_clear(trigger);
}