/* panic.h: what happens when the kernel cannot go on. */

#ifndef PANIC_H
#define PANIC_H

#include "isr.h"

void panic(const char *message);
void panic_exception(const t_registers *regs);
void panic_save_stack(void);
void panic_print_saved_stack(void);

/* boot/halt.asm: clears the registers and stops the CPU; never returns */
void cpu_halt_clean(void);

#endif
