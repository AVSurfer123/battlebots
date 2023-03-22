#include "rpi.h"
#include "servo.h"


void notmain() {
  printk("beginning servo test\n");
  // unimplemented();

  int pin = 19;
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
}