#include "user.h"

void main(void) {
  /*
  *((volatile int *)0x80300000) =
      0x1234; // invalid write to kernel memory space from user process
  for (;;)
    ;
  */
  printf("\nHello World from shell!\n");
}
