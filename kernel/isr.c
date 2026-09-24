/* isr.c — the C side of every interrupt: exceptions, IRQs and the syscall vector. */

#include "isr.h"
#include "pic.h"
#include "signal.h"
#include "panic.h"
#include "syscall.h"
#include "printk.h"

static const char *g_exception_names[EXCEPTION_COUNT] = {
	"division by zero",
	"debug",
	"non maskable interrupt",
	"breakpoint",
	"overflow",
	"bound range exceeded",
	"invalid opcode",
	"coprocessor not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid task state segment",
	"segment not present",
	"stack fault",
	"general protection fault",
	"page fault",
	"reserved",
	"math fault",
	"alignment check",
	"machine check",
	"SIMD floating-point exception",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved",
};

/* One callback per IRQ line; NULL means "nobody cares" */
static t_irq_handler g_irq_handlers[IRQ_COUNT];

const char *exception_name(uint32_t vector)
{
	if (vector >= EXCEPTION_COUNT)
		return ("not an exception");
	return (g_exception_names[vector]);
}

/* Registers a driver for an IRQ line and lets the PIC deliver it. */
void irq_register(uint8_t irq, t_irq_handler handler)
{
	if (irq >= IRQ_COUNT)
		return ;
	g_irq_handlers[irq] = handler;
	pic_unmask(irq);
}

/* Traps report something that already happened: the CPU saved the address of the NEXT */
/* instruction, so returning is fine. Faults saved the FAULTING instruction: returning */
/* would run it again and fault forever. Hence traps resume and faults end in a panic. */
static bool_t is_trap(uint32_t vector)
{
	return (vector == 1 || vector == 3 || vector == 4);
}

static void handle_exception(t_registers *regs)
{
	printk("\n[exception %u] %s at eip=%p, error code 0x%x\n", regs->int_no,
		exception_name(regs->int_no), (void *)regs->eip, regs->err_code);
	signal_raise(signal_for_exception(regs->int_no));
	if (!is_trap(regs->int_no))
		panic_exception(regs);
}

static void handle_irq(t_registers *regs)
{
	uint8_t irq = (uint8_t)(regs->int_no - IRQ_BASE);

	if (g_irq_handlers[irq] != NULL)
		g_irq_handlers[irq](regs);
	/* Without this acknowledgement the PIC never sends that line again */
	pic_send_eoi(irq);
}

/* Called by isr_common (boot/isr.asm) for every vector that has a stub. */
void interrupt_handler(t_registers *regs)
{
	if (regs->int_no < EXCEPTION_COUNT)
		handle_exception(regs);
	else if (regs->int_no < IRQ_BASE + IRQ_COUNT)
		handle_irq(regs);
	else if (regs->int_no == SYSCALL_VECTOR)
		syscall_handler(regs);
}
