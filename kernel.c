#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[], __free_ram[], __free_ram_end[],
    __kernel_base[]; // [] - returns start address and not the 0th byte

// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x1000000
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory"); /* indicates that ecall might access memory
                                       to avoid compiler making optimizations */

  return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */); // eid = 1, fid = 0
}

long getchar(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
  return ret.error; // -1 if no input
}

paddr_t alloc_pages(uint32_t n) {
  static paddr_t next_paddr =
      (paddr_t)__free_ram; // value retained - initialized only once - allocate
                           // sequentially starting from __free_ram
  paddr_t paddr = next_paddr;
  next_paddr += n * PAGE_SIZE;

  if (next_paddr > (paddr_t)__free_ram_end)
    PANIC("out of memory!");

  memset((void *)paddr, 0, n * PAGE_SIZE);           // fill memory area with 0s
  printf("allocated memory address: 0x%x\n", paddr); // DEBUG
  return paddr;
}

void map_page(uint32_t *table1, vaddr_t vaddr, paddr_t paddr, uint32_t flags) {
  if (!is_aligned(vaddr, PAGE_SIZE)) { // TODO: revisit
    PANIC("unaligned vaddr %x", vaddr);
  }

  if (!is_aligned(paddr, PAGE_SIZE)) {
    PANIC("unaligned paddr %x", paddr);
  }

  uint32_t vpn1 =
      (vaddr >> 22) & 0x3ff; // right shift 22bits + bit mask 11_1111_1111
  if ((table1[vpn1] & PAGE_V) == 0) {
    // Create the non-existent 2nd level page table.
    uint32_t pt_paddr = alloc_pages(1);
    table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) |
                   PAGE_V; // lower bits (0-9) used for flags
  }

  // Set the 2nd level page table entry to map the physical page.
  uint32_t vpn0 =
      (vaddr >> 12) & 0x3ff; // right shift 12bits + bit mask 11_1111_1111
  uint32_t *table0 =
      (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE); // don't need flags here
  table0[vpn0] =
      ((paddr / PAGE_SIZE) << 10) | flags |
      PAGE_V; // make room for flags
              // entry contains the physical page number and not address
}

// __attribute__((naked)) is very important! - compile without any compiler
// generated function prologue/epilogue code
__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__(
      "csrw sepc, %[sepc]        \n"
      "csrw sstatus, %[sstatus]  \n"
      "sret                      \n" // jumps to sepc - switches to U-mode
      :
      : [sepc] "r"(USER_BASE), [sstatus] "r"(SSTATUS_SPIE));
}

__attribute__((naked)) __attribute__((aligned(4))) void
kernel_entry(void) { // entrypoint to kernel
  __asm__ __volatile__(
      // save registers since they might be overwritten during trap handling
      // "csrw sscratch, sp\n" // using the global stack
      // Retrieve the kernel stack of the running process from sscratch.
      "csrrw sp, sscratch, sp\n" // using stack of running process
      "addi sp, sp, -4 * 31\n"   // 31 = 0 ~ 30
      "sw ra,  4 * 0(sp)\n"
      "sw gp,  4 * 1(sp)\n"
      "sw tp,  4 * 2(sp)\n"
      "sw t0,  4 * 3(sp)\n"
      "sw t1,  4 * 4(sp)\n"
      "sw t2,  4 * 5(sp)\n"
      "sw t3,  4 * 6(sp)\n"
      "sw t4,  4 * 7(sp)\n"
      "sw t5,  4 * 8(sp)\n"
      "sw t6,  4 * 9(sp)\n"
      "sw a0,  4 * 10(sp)\n"
      "sw a1,  4 * 11(sp)\n"
      "sw a2,  4 * 12(sp)\n"
      "sw a3,  4 * 13(sp)\n"
      "sw a4,  4 * 14(sp)\n"
      "sw a5,  4 * 15(sp)\n"
      "sw a6,  4 * 16(sp)\n"
      "sw a7,  4 * 17(sp)\n"
      "sw s0,  4 * 18(sp)\n"
      "sw s1,  4 * 19(sp)\n"
      "sw s2,  4 * 20(sp)\n"
      "sw s3,  4 * 21(sp)\n"
      "sw s4,  4 * 22(sp)\n"
      "sw s5,  4 * 23(sp)\n"
      "sw s6,  4 * 24(sp)\n"
      "sw s7,  4 * 25(sp)\n"
      "sw s8,  4 * 26(sp)\n"
      "sw s9,  4 * 27(sp)\n"
      "sw s10, 4 * 28(sp)\n"
      "sw s11, 4 * 29(sp)\n"

      // process's stack is used to store registers, and the kernel’s stack is
      // temporarily saved in sscratch

      // Retrieve and save the sp at the time of exception.
      "csrr a0, sscratch\n"
      "sw a0,  4 * 30(sp)\n" // store sp at the bottom most section of stack

      // Reset the kernel stack.
      "addi a0, sp, 4 * 31\n" // calculate address for kernel's stack
      "csrw sscratch, a0\n"   // TODO: revisit

      "mv a0, sp\n" // pass sp as argument to trap_handler - struct trap_frame
      "call handle_trap\n" // call trap handler

      // restore saved registers from the stack - reverting CPU state
      "lw ra,  4 * 0(sp)\n"
      "lw gp,  4 * 1(sp)\n"
      "lw tp,  4 * 2(sp)\n"
      "lw t0,  4 * 3(sp)\n"
      "lw t1,  4 * 4(sp)\n"
      "lw t2,  4 * 5(sp)\n"
      "lw t3,  4 * 6(sp)\n"
      "lw t4,  4 * 7(sp)\n"
      "lw t5,  4 * 8(sp)\n"
      "lw t6,  4 * 9(sp)\n"
      "lw a0,  4 * 10(sp)\n"
      "lw a1,  4 * 11(sp)\n"
      "lw a2,  4 * 12(sp)\n"
      "lw a3,  4 * 13(sp)\n"
      "lw a4,  4 * 14(sp)\n"
      "lw a5,  4 * 15(sp)\n"
      "lw a6,  4 * 16(sp)\n"
      "lw a7,  4 * 17(sp)\n"
      "lw s0,  4 * 18(sp)\n"
      "lw s1,  4 * 19(sp)\n"
      "lw s2,  4 * 20(sp)\n"
      "lw s3,  4 * 21(sp)\n"
      "lw s4,  4 * 22(sp)\n"
      "lw s5,  4 * 23(sp)\n"
      "lw s6,  4 * 24(sp)\n"
      "lw s7,  4 * 25(sp)\n"
      "lw s8,  4 * 26(sp)\n"
      "lw s9,  4 * 27(sp)\n"
      "lw s10, 4 * 28(sp)\n"
      "lw s11, 4 * 29(sp)\n"
      "lw sp,  4 * 30(sp)\n"
      "sret\n"); // return to value stored in sepc (program counter)
}

__attribute__((naked)) void
switch_context(uint32_t *prev_sp,
               uint32_t *next_sp) { // acts as callee function
  __asm__ __volatile__(
      // different stack space for each process
      // callee-saved -> callee function using these registers must save before
      // using and restore before returning
      // caller assumes these registers will retain original values
      // Save callee-saved registers onto the current process's stack.
      "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers
                               // in the current process's stack
      "sw ra,  0  * 4(sp)\n"   // Save callee-saved registers only
      "sw s0,  1  * 4(sp)\n"
      "sw s1,  2  * 4(sp)\n"
      "sw s2,  3  * 4(sp)\n"
      "sw s3,  4  * 4(sp)\n"
      "sw s4,  5  * 4(sp)\n"
      "sw s5,  6  * 4(sp)\n"
      "sw s6,  7  * 4(sp)\n"
      "sw s7,  8  * 4(sp)\n"
      "sw s8,  9  * 4(sp)\n"
      "sw s9,  10 * 4(sp)\n"
      "sw s10, 11 * 4(sp)\n"
      "sw s11, 12 * 4(sp)\n"

      // Switch the stack pointer.
      // a0 = *prev_sp a1 = *next_sp
      "sw sp, (a0)\n" // *prev_sp = sp;
      "lw sp, (a1)\n" // Switch stack pointer (sp) here

      // Restore callee-saved registers from the next process's stack.
      "lw ra,  0  * 4(sp)\n" // Restore callee-saved registers only
      "lw s0,  1  * 4(sp)\n"
      "lw s1,  2  * 4(sp)\n"
      "lw s2,  3  * 4(sp)\n"
      "lw s3,  4  * 4(sp)\n"
      "lw s4,  5  * 4(sp)\n"
      "lw s5,  6  * 4(sp)\n"
      "lw s6,  7  * 4(sp)\n"
      "lw s7,  8  * 4(sp)\n"
      "lw s8,  9  * 4(sp)\n"
      "lw s9,  10 * 4(sp)\n"
      "lw s10, 11 * 4(sp)\n"
      "lw s11, 12 * 4(sp)\n"
      "addi sp, sp, 13 * 4\n" // We've popped 13 4-byte registers from the next
                              // process's stack
      "ret\n"); // jump to ra register - proc_b_entry - for the 1st run
}

struct process procs[PROCS_MAX]; // All process control structures.

struct process *create_process(uint32_t pc, const void *image,
                               size_t image_size) {
  // Find an unused process control structure.
  struct process *proc = NULL;
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }

  if (!proc)
    PANIC("no free process slots");

  // Stack callee-saved registers. These register values will be restored in
  // the first context switch in switch_context.
  uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
  *--sp = 0;            // s11
  *--sp = 0;            // s10
  *--sp = 0;            // s9
  *--sp = 0;            // s8
  *--sp = 0;            // s7
  *--sp = 0;            // s6
  *--sp = 0;            // s5
  *--sp = 0;            // s4
  *--sp = 0;            // s3
  *--sp = 0;            // s2
  *--sp = 0;            // s1
  *--sp = 0;            // s0
  *--sp = (uint32_t)pc; // ra

  uint32_t *page_table = (uint32_t *)alloc_pages(1);

  // Map kernel pages such that it can access anything
  for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end;
       paddr += PAGE_SIZE) {
    map_page(page_table, paddr, paddr,
             PAGE_R | PAGE_W | PAGE_X); // vaddr = paddr
  }

  // Map & allocate user pages for user program
  if (image) {
    for (uint32_t image_offset = 0; image_offset < image_size;
         image_offset += PAGE_SIZE) {
      paddr_t page = alloc_pages(1);

      // Handle the case where the data to be copied is smaller than the
      // page size.
      size_t remaining = image_size - image_offset;
      size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

      // Fill and map the page.
      memcpy((void *)page, image + image_offset, copy_size);
      map_page(page_table, USER_BASE + image_offset, page,
               PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }
  }

  // Initialize fields.
  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t)sp;
  proc->page_table = page_table;
  return proc;
}

void delay(void) { // busy waiting
  for (int i = 0; i < 200000000; i++)
    __asm__ __volatile__("nop"); // do nothing
}

struct process *proc_a;
struct process *proc_b;
struct process *proc_c;

struct process *current_proc; // currently running process
struct process *idle_proc; // process to run if there are no runnable processes

void yield(void) {
  // Search for a runnable process
  struct process *next = idle_proc;
  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }

  // If there's no runnable process other than the current one, return and
  // continue processing
  if (next == current_proc)
    return;

  __asm__ __volatile__(
      "sfence.vma\n" // clear TLB & fence memory
      "csrw satp, %[satp]\n"
      "sfence.vma\n"
      "csrw sscratch, %[sscratch]\n"
      :
      : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table /
                                PAGE_SIZE)), // PPN bits in satp register 21-0
        [sscratch] "r"((uint32_t)&next->stack[sizeof(
            next->stack)])); // store TOS of next process to help in exception
                             // handling

  // Context switch
  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}

void proc_a_entry(void) { // process A entrypoint
  printf("starting process A\n");
  while (1) {
    putchar('A');
    *((volatile int *)0x80300000) =
        0x1234; // valid write to kernel memory space from kernel process
    delay();
    yield();
  }
}

void proc_b_entry(void) { // process B entrypoint
  printf("starting process B\n");
  while (1) {
    putchar('B');
    delay();
    yield();
  }
}

void handle_syscall(struct trap_frame *frame) {
  switch (frame->a3) {
  case SYS_EXIT:
    printf("process %d exited\n", current_proc->pid);
    current_proc->state = PROC_EXITED;
    yield();
    PANIC("unreachable");
  case SYS_GETCHAR:
    while (1) {
      long ch = getchar();
      if (ch >= 0) {
        frame->a0 = ch;
        break;
      }
      yield(); // don't block CPU
    }
    break;
  case SYS_PUTCHAR:
    putchar(frame->a0);
    break;
  default:
    PANIC("unexpected syscall a3=%x\n", frame->a3);
  }
}

void handle_trap(
    struct trap_frame
        *frame /* trap_frame was passed as reg a0 */) { // trap handler function
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);
  if (scause == SCAUSE_ECALL) {
    handle_syscall(frame);
    user_pc += 4;
  } else {
    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
          user_pc);
  }

  WRITE_CSR(sepc, user_pc); // return back to sepc
}

void kernel_main(void) { // what to be done by kernel
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

  WRITE_CSR( // placement is important to catch exceptions
      stvec,
      (uint32_t)kernel_entry); // register exception handler in stvec register
  // __asm__ __volatile__("unimp"); // trigger illegal instruction

  printf("\nHello %s", "World!");
  printf("\nHi I'm %s", "Avishek");
  printf("\n1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

  paddr_t paddr0 = alloc_pages(2);
  paddr_t paddr1 = alloc_pages(1);
  printf("alloc_pages test: paddr0=%x\n", paddr0);
  printf("alloc_pages test: paddr1=%x\n", paddr1);

  idle_proc = create_process((uint32_t)NULL, NULL, 0);
  idle_proc->pid = -1; // idle
  current_proc =
      idle_proc; // ensures execution context of boot process is saved and
                 // restored when all processes finish execution

  // proc_a = create_process((uint32_t)proc_a_entry, NULL, 0); // kernel process
  // proc_b = create_process((uint32_t)proc_b_entry, NULL, 0); // kernel process
  proc_c = create_process((uint32_t)user_entry,
                          _binary_shell_bin_start, // user process (shell)
                          (size_t)_binary_shell_bin_size);
  yield();

  PANIC("switched to idle process");
  // printf("unreachable here!\n");

  for (;;) {
    __asm__ __volatile__("wfi"); // wait-for-interrupt to save power
  }
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void
boot(void) { // start booting OS from here
  __asm__ __volatile__(
      "mv sp, %[stack_top]\n" // set the stack pointer
      "j kernel_main\n"       // jump to the kernel main function
      :
      : [stack_top] "r"(
          __stack_top) // pass the stack top address as %[stack_top]
  );
}
