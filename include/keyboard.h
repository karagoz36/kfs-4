/* keyboard.h: PS/2 keyboard driver, fed by IRQ 1. */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

void        keyboard_init(void);
char        keyboard_getchar(void);
size_t      keyboard_get_line(char *buf, size_t size);
bool_t      keyboard_set_layout(const char *name);
const char *keyboard_layout_name(void);
void        keyboard_reboot(void);

#endif
