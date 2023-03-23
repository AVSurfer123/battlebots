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

  int pin = 12;
  init_servo(pin);

  int start = timer_get_usec();
  int elapsed = 0;
  while (elapsed < 20) {
      elapsed = (timer_get_usec() - start) / 500000;
      // if (elapsed < 2)
      //   set_servo(pin, 0);
      // else if (elapsed < 4)
      //   set_servo(pin, 90);
      // else if (elapsed < 6)
      //   set_servo(pin, 180);
      // else if (elapsed < 8)
      //   set_servo(pin, 90);
      // else
      //   set_servo(pin, 0);

      // Squeeze sequence
      if (elapsed % 2 == 0) {
        set_servo(pin, 30);
      }
      else {
        set_servo(pin, 120);
      }
  }


}