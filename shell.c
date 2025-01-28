#include "user.h"

void main(void) {
  /*
  *((volatile int *)0x80300000) =
      0x1234; // invalid write to kernel memory space from user process
  for (;;)
    ;
  */

  // printf("\nHello World from shell!\n");

  // test not saving sscratch during context switching
  // __asm__ __volatile__("li sp, 0xdeadbeef\n" // set an invalid address as sp
  //                      "unimp");             // trigger exception

  while (1) {
  prompt:
    printf("> ");
    char cmdline[128];
    for (int i = 0;; i++) {
      char ch = getchar();
      putchar(ch);
      if (i == sizeof(cmdline) - 1) {
        printf("command line too long\n");
        goto prompt;
      } else if (ch == '\r') {
        printf("\n");
        cmdline[i] = '\0';
        break;
      } else {
        cmdline[i] = ch;
      }
    }

    if (strcmp(cmdline, "hello") == 0) {
      printf("Hello world from shell!\n");
    } else if (strcmp(cmdline, "exit") == 0) {
      exit();
    } else {
      printf("unknown command: %s\n", cmdline);
    }
  }
}
