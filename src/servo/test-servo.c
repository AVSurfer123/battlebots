#include "rpi.h"
#include "servo.h"


void notmain() {
  printk("beginning servo test\n");
  // unimplemented();

  int pin = 0;
  init_servo(pin);

  int start = timer_get_usec();
  int elapsed = 0;
  while (elapsed < 20) {
      elapsed = (timer_get_usec() - start) / 1000000;
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
        set_servo(pin, 0);
      }
      else {
        set_servo(pin, 180);
      }
  }


}
