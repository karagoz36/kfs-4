/* printk.h: supports %c %s %d %i %u %x %p %% and a zero-padding width such as %08x. */

#ifndef PRINTK_H
#define PRINTK_H

void printk(const char *format, ...);

#endif
