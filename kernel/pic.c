/* pic.c — programming the two 8259 PICs. */

#include "pic.h"
#include "io.h"
#include "isr.h"

/* Each PIC has a command port and a data port */
#define PIC1_COMMAND 0x20   /* master: IRQ 0..7 */
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0   /* slave: IRQ 8..15, wired to the master's line 2 */
#define PIC2_DATA    0xA1

#define ICW1_INIT 0x11   /* start the initialisation sequence, 4 words follow */
#define ICW4_8086 0x01   /* 8086 mode */
#define PIC_EOI   0x20   /* "end of interrupt" command */

#define IRQ_CASCADE 2    /* the master line the slave is connected to */

/* At boot the BIOS maps IRQ 0..7 onto vectors 8..15, right on top of the CPU exceptions: */
/* the timer would look like a double fault. The sequence below moves them to 32..47. */
void pic_init(void)
{
	outb(PIC1_COMMAND, ICW1_INIT);
	outb(PIC2_COMMAND, ICW1_INIT);
	outb(PIC1_DATA, IRQ_BASE);          /* ICW2: first vector of each PIC */
	outb(PIC2_DATA, IRQ_BASE + 8);
	outb(PIC1_DATA, 1 << IRQ_CASCADE);  /* ICW3: master, slave is on line 2 */
	outb(PIC2_DATA, IRQ_CASCADE);       /* ICW3: slave, its identity is 2 */
	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

	/* Mask everything: a line is opened when a driver registers for it */
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);
}

/* Clears the mask bit of one IRQ line. Bit set = masked, so the bit is cleared. */
void pic_unmask(uint8_t irq)
{
	uint16_t port = PIC1_DATA;

	if (irq >= 8)
	{
		port = PIC2_DATA;
		irq -= 8;
		pic_unmask(IRQ_CASCADE);  /* a slave IRQ also needs the master's line 2 */
	}
	outb(port, inb(port) & (uint8_t)~(1 << irq));
}

/* Tells the PIC the IRQ has been handled; a slave IRQ went through both chips. */
void pic_send_eoi(uint8_t irq)
{
	if (irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}
