/* panic.c — stopping the kernel in a readable way: registers, stack, then halt. */

#include "panic.h"
#include "console.h"
#include "printk.h"

/* boot/boot.asm */
extern uint8_t stack_top[];

#define SAVED_WORDS 32   /* 128 bytes: what fits on the screen next to the registers */

/* A copy of the top of the stack, taken before anything else runs and moves it. */
static uint32_t g_saved_stack[SAVED_WORDS];
static uint32_t g_saved_esp;
static size_t   g_saved_count;

/* Copies the words between esp and the top of the stack (at most SAVED_WORDS). */
void panic_save_stack(void)
{
	const uint32_t *word;

	__asm__ volatile("mov %%esp, %0" : "=r"(g_saved_esp));
	word = (const uint32_t *)g_saved_esp;
	g_saved_count = 0;
	while (g_saved_count < SAVED_WORDS
		&& (const uint8_t *)&word[g_saved_count] < stack_top)
	{
		g_saved_stack[g_saved_count] = word[g_saved_count];
		g_saved_count++;
	}
}

/* Prints the copy, four words per line, the way 'stack' does for the live stack. */
void panic_print_saved_stack(void)
{
	size_t i = 0;

	printk("stack saved at esp=%p (%u words):\n", (void *)g_saved_esp,
		(uint32_t)g_saved_count);
	while (i < g_saved_count)
	{
		if (i % 4 == 0)
			printk("%08x: ", g_saved_esp + i * sizeof(uint32_t));
		printk(" %08x", g_saved_stack[i]);
		i++;
		if (i % 4 == 0 || i == g_saved_count)
			printk("\n");
	}
}

/* Never returns: everything after this line runs with interrupts off. */
void panic(const char *message)
{
	__asm__ volatile("cli");
	panic_save_stack();
	console_set_color(VGA_WHITE, VGA_RED);
	printk("KERNEL PANIC: %s", message);
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	printk("\n");
	panic_print_saved_stack();
	printk("System halted.\n");
	cpu_halt_clean();
}

/* A panic caused by a CPU exception: the frame tells where and in which state. */
void panic_exception(const t_registers *regs)
{
	printk("eax=%08x ebx=%08x ecx=%08x edx=%08x\n", regs->eax, regs->ebx,
		regs->ecx, regs->edx);
	printk("esi=%08x edi=%08x ebp=%08x esp=%08x\n", regs->esi, regs->edi,
		regs->ebp, regs->esp);
	printk("eip=%08x cs=%04x eflags=%08x\n", regs->eip, regs->cs, regs->eflags);
	panic(exception_name(regs->int_no));
}
