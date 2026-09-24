/* timer.h: the PIT (programmable interval timer) ticking on IRQ 0. */

#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define TIMER_HZ 100   /* one tick every 10 ms */

void     timer_init(void);
uint32_t timer_ticks(void);

#endif
