/* gdt.h: the table of memory segments at 0x800: 6 segments plus the required null entry. */

#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_ENTRIES 7

/* One 8-byte descriptor; base and limit are scattered for 80286 reasons, hence 'packed'. */
typedef struct s_gdt_entry
{
	uint16_t limit_low;    /* limit bits 0..15 */
	uint16_t base_low;     /* base bits 0..15 */
	uint8_t  base_mid;     /* base bits 16..23 */
	uint8_t  access;       /* present, privilege level, code/data, read/write */
	uint8_t  granularity;  /* limit bits 16..19 (low nibble) + flags (high nibble) */
	uint8_t  base_high;    /* base bits 24..31 */
}	__attribute__((packed)) t_gdt_entry;

/* The table itself: 7 descriptors at 0x800. The address comes from linker.ld */
extern t_gdt_entry gdt_table[GDT_ENTRIES];

void gdt_init(void);
void gdt_print(void);

#endif
