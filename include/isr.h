/* isr.h: what the interrupt stubs (boot/isr.asm) hand to C, and the IRQ callback table. */

#ifndef ISR_H
#define ISR_H

#include "types.h"

#define EXCEPTION_COUNT 32   /* vectors 0..31 are reserved for the CPU */
#define IRQ_BASE        32   /* the PIC is remapped so that IRQ 0 is vector 32 */
#define IRQ_COUNT       16

#define IRQ_TIMER    0
#define IRQ_KEYBOARD 1

/* The stack as isr_common leaves it, lowest address first. */
typedef struct s_registers
{
	uint32_t edi;       /* pushed by 'pusha' (in reverse order of the instruction) */
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;       /* value before 'pusha', not very useful */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t int_no;    /* pushed by the stub */
	uint32_t err_code;  /* pushed by the CPU, or a fake 0 from the stub */
	uint32_t eip;       /* pushed by the CPU when the interrupt fired */
	uint32_t cs;
	uint32_t eflags;
}	t_registers;

typedef void (*t_irq_handler)(t_registers *regs);

void        irq_register(uint8_t irq, t_irq_handler handler);
const char *exception_name(uint32_t vector);

#endif
