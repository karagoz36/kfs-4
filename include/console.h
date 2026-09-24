/* console.h: four virtual screens; only the active one is blitted to the framebuffer. */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "vga.h"

#define CONSOLE_COUNT 4

void   console_init(void);
void   console_switch(size_t index);
void   console_set_color(uint8_t fg, uint8_t bg);
void   console_clear(void);
void   console_putchar(char c);
void   console_write(const char *s);

#endif
