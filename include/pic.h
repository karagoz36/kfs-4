/* pic.h: the two 8259 interrupt controllers that turn hardware lines into IRQ 0..15. */

#ifndef PIC_H
#define PIC_H

#include "types.h"

void pic_init(void);
void pic_unmask(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif
