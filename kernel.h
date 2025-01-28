#pragma once
#include "common.h"

// infinite loop to halt processing
#define PANIC(fmt, ...)                                                        \
  do {                                                                         \
    printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
    while (1) {                                                                \
    }                                                                          \
  } while (0)

#define READ_CSR(reg)                                                          \
  ({                                                                           \
    unsigned long __tmp;                                                       \
    __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                      \
    __tmp;                                                                     \
  })

#define WRITE_CSR(reg, value)                                                  \
  do {                                                                         \
    uint32_t __tmp = (value);                                                  \
    __asm__ __volatile__("csrw " #reg ", %0" : : "r"(__tmp));                  \
  } while (0)

struct sbiret {
  long error;
  long value;
};

struct trap_frame {
  uint32_t ra;
  uint32_t gp;
  uint32_t tp;
  uint32_t t0;
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;
  uint32_t t4;
  uint32_t t5;
  uint32_t t6;
  uint32_t a0;
  uint32_t a1;
  uint32_t a2;
  uint32_t a3;
  uint32_t a4;
  uint32_t a5;
  uint32_t a6;
  uint32_t a7;
  uint32_t s0;
  uint32_t s1;
  uint32_t s2;
  uint32_t s3;
  uint32_t s4;
  uint32_t s5;
  uint32_t s6;
  uint32_t s7;
  uint32_t s8;
  uint32_t s9;
  uint32_t s10;
  uint32_t s11;
  uint32_t sp;
} __attribute__((packed));

#define PROCS_MAX 8 // Maximum number of processes

#define PROC_UNUSED 0   // Unused process control structure
#define PROC_RUNNABLE 1 // Runnable process
#define PROC_EXITED 2   // Exited process

struct process {
  int pid;              // Process ID
  int state;            // Process state: PROC_UNUSED or PROC_RUNNABLE
  vaddr_t sp;           // Stack pointer pointing to kernel stack
  uint8_t stack[8192];  // Kernel stack of the process - 8KB
  uint32_t *page_table; // pointer to 1st level page table
};

#define SATP_SV32 (1u << 31) // 32bit unsigned int - bit 31 = 1 - satp register
#define PAGE_V (1 << 0)      // "Valid" bit (entry is enabled) - flags
#define PAGE_R (1 << 1)      // Readable
#define PAGE_W (1 << 2)      // Writable - left shift 2bits
#define PAGE_X (1 << 3)      // Executable - left shift 3bits
#define PAGE_U (1 << 4)      // User (accessible in user mode)

#define SSTATUS_SPIE (1 << 5) // switch mode from S to U
#define SCAUSE_ECALL 8        // environment call from U-mode
