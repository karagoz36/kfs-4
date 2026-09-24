/* timer.c — the PIT: a counter that raises IRQ 0 at a fixed rate. */

#include "timer.h"
#include "io.h"
#include "isr.h"

#define PIT_CHANNEL0 0x40   /* the channel wired to IRQ 0 */
#define PIT_COMMAND  0x43
#define PIT_SET_RATE 0x36   /* channel 0, low then high byte, square wave, binary */
#define PIT_BASE_HZ  1193182

/* 'volatile': written by the IRQ, read by the main loop */
static volatile uint32_t g_ticks = 0;

static void timer_irq(t_registers *regs)
{
	(void)regs;
	g_ticks++;
}

/* The PIT counts down from 'divisor' at 1.19 MHz and fires when it reaches 0. */
void timer_init(void)
{
	uint16_t divisor = PIT_BASE_HZ / TIMER_HZ;

	outb(PIT_COMMAND, PIT_SET_RATE);
	outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
	outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
	irq_register(IRQ_TIMER, timer_irq);
}

uint32_t timer_ticks(void)
{
	return (g_ticks);
}
