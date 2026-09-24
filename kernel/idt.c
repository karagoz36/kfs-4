/* idt.c — filling the Interrupt Descriptor Table and handing it to the CPU with 'lidt'. */

#include "idt.h"
#include "isr.h"
#include "syscall.h"
#include "printk.h"

/* Flags: present (0x80) | privilege level (0x00 or 0x60) | 32-bit interrupt gate (0x0E).
 * An interrupt gate clears IF on entry, so a handler cannot be interrupted by an IRQ. */
#define GATE_KERNEL 0x8E   /* only the kernel (or the hardware) may raise it */
#define GATE_USER   0xEE   /* 'int' from ring 3 is allowed too: for system calls */

#define KERNEL_CODE_SELECTOR 0x08   /* entry 1 of our GDT */

/* What 'lidt' reads: size of the table minus one, then its address */
typedef struct s_idt_ptr
{
	uint16_t limit;
	uint32_t base;
}	__attribute__((packed)) t_idt_ptr;

static t_idt_entry g_idt[IDT_ENTRIES];

/* boot/isr.asm: the addresses of the 48 stubs (exceptions then IRQs) and the syscall one */
extern const uint32_t isr_stub_table[EXCEPTION_COUNT + IRQ_COUNT];
extern void           isr128(void);

static void idt_set_gate(size_t vector, uint32_t handler, uint8_t flags)
{
	g_idt[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
	g_idt[vector].selector    = KERNEL_CODE_SELECTOR;
	g_idt[vector].zero        = 0;
	g_idt[vector].flags       = flags;
	g_idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

/* Gates left all zeros are "not present": raising one triggers a general protection fault, */
/* which our handler prints. That is safer than jumping to address 0. */
void idt_init(void)
{
	t_idt_ptr ptr;
	size_t    vector = 0;

	while (vector < EXCEPTION_COUNT + IRQ_COUNT)
	{
		idt_set_gate(vector, isr_stub_table[vector], GATE_KERNEL);
		vector++;
	}
	idt_set_gate(SYSCALL_VECTOR, (uint32_t)isr128, GATE_USER);

	ptr.limit = sizeof(g_idt) - 1;
	ptr.base  = (uint32_t)g_idt;
	__asm__ volatile("lidt %0" : : "m"(ptr));
}

/* Shows the gates that are in use. */
void idt_print(void)
{
	size_t   vector = 0;
	size_t   present = 0;
	uint32_t handler;

	while (vector < IDT_ENTRIES)
	{
		if (g_idt[vector].flags != 0)
			present++;
		vector++;
	}
	printk("IDT at %p: %u gates, %u present\n", (void *)g_idt,
		(uint32_t)IDT_ENTRIES, (uint32_t)present);
	printk("  0x00-0x1f  CPU exceptions      %s\n", exception_name(0));
	printk("  0x20-0x2f  IRQ 0..15           timer, keyboard...\n");
	handler = g_idt[SYSCALL_VECTOR].offset_low
		| ((uint32_t)g_idt[SYSCALL_VECTOR].offset_high << 16);
	printk("  0x80       system calls        handler %p, flags 0x%02x\n",
		(void *)handler, (uint32_t)g_idt[SYSCALL_VECTOR].flags);
}
