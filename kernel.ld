ENTRY(boot) /* boot function is the entry point of the kernel (program) */

SECTIONS {
  . = 0x80200000; /* base address - dot represents current address - auto increments */
  __kernel_base = .; /* starting address used by kernel */

  .text :{
    KEEP(*(.text.boot)); /* place at the beginning */
    *(.text .text.*); /* from all files (*) place (.text) and all sections starting with (.text.*) */
  }

  .rodata : ALIGN(4) { /* adjust to 4-byte boundary */
    *(.rodata .rodata.*);
  }

  .data : ALIGN(4) {
    *(.data .data.*);
  }

  .bss : ALIGN(4) {
    __bss = .; /* assign current address to __bss symbol */
    *(.bss .bss.* .sbss .sbss.*);
    __bss_end = .;
  }

  . = ALIGN(4); 
  . += 128 * 1024; /* advance the current address by 128KB */
  __stack_top = .; /* kernel stack */

  . = ALIGN(4096); /* aligned to 4KB boundary */
  __free_ram = .;
  . += 64 * 1024 * 1024; /* 64MB RAM - arbitrary */
  __free_ram_end = .;
}
