/* idt.h: the Interrupt Descriptor Table, 256 gates telling the CPU where to jump for each vector. */

#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRIES 256

/* One 8-byte gate; the handler address is split in two like a GDT base. */
typedef struct s_idt_entry
{
	uint16_t offset_low;   /* handler address bits 0..15 */
	uint16_t selector;     /* code segment to run the handler in (kernel code, 0x08) */
	uint8_t  zero;         /* unused, must be 0 */
	uint8_t  flags;        /* present, privilege level, gate type */
	uint16_t offset_high;  /* handler address bits 16..31 */
}	__attribute__((packed)) t_idt_entry;

void idt_init(void);
void idt_print(void);

#endif
