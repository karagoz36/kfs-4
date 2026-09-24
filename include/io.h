/* io.h: devices in the I/O address space, reached only by 'in' and 'out' (no C equivalent). */

#ifndef IO_H
#define IO_H

#include "types.h"

/* Writes one byte to the given port. "a" = al register, "Nd" = dx or immediate */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Reads one byte from the given port. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return (value);
}

#endif
