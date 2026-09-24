/* main.c — the kernel's entry point on the C side. */

#include "console.h"
#include "printk.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "signal.h"
#include "shell.h"

/* Header, banner and shell prompt of one virtual screen. */
static void draw_screen(size_t index)
{
	console_set_color(VGA_BLACK, VGA_LIGHT_GREY);
	printk(" KFS-4  screen %u/%u ", (uint32_t)(index + 1),
		(uint32_t)CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	printk("\n\n42\n\n");
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	printk("Kernel From Scratch 4 - GDT at %p, IDT loaded, IRQ 0 and 1 live.\n",
		(void *)gdt_table);
	console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	printk("Type 'help' for the commands. Switch screen: Alt+1..%d.\n\n",
		CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	shell_init();
}

/* Every virtual screen (bonus) gets the same content, then screen 1 is shown. */
static void draw_screens(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		console_switch(i);
		draw_screen(i);
		i++;
	}
	console_switch(0);
}

/* kernel_main never returns: the bootloader is gone, there is nowhere to return to. */
void kernel_main(void)
{
	char c;

	gdt_init();       /* our segments instead of GRUB's */
	console_init();
	idt_init();       /* where the CPU jumps for each exception and IRQ */
	pic_init();       /* IRQ 0..15 moved to vectors 32..47, all lines masked */
	timer_init();     /* IRQ 0: the clock used to schedule signals */
	keyboard_init();  /* IRQ 1: keys go to a queue instead of being polled */
	draw_screens();
	__asm__ volatile("sti");  /* from here on the hardware can interrupt us */

	/* The CPU sleeps ('hlt') until an interrupt wakes it: a key, or the timer 100 times */
	/* a second. Signals are delivered here, outside of any interrupt handler. */
	while (TRUE)
	{
		signal_dispatch_pending();
		c = keyboard_getchar();
		if (c != 0)
			shell_input(c);
		else
			__asm__ volatile("hlt");
	}
}
