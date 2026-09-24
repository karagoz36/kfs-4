/* vga.c — VGA text mode driver. It does two things: */

#include "vga.h"
#include "io.h"

/* CRT controller: */
#define CRTC_INDEX_PORT 0x3D4
#define CRTC_DATA_PORT  0x3D5

#define CRTC_CURSOR_START 0x0A  /* top scanline of the cursor + hide bit */
#define CRTC_CURSOR_END   0x0B  /* bottom scanline of the cursor */
#define CRTC_CURSOR_HIGH  0x0E  /* high byte of the cursor position */
#define CRTC_CURSOR_LOW   0x0F  /* low byte of the cursor position */

/* Updates a single cell of the framebuffer. */
void vga_write_cell(size_t index, uint16_t cell)
{
	if (index < VGA_WIDTH * VGA_HEIGHT)
		VGA_MEMORY[index] = cell;
}

/* Copies a whole 80x25 buffer onto the screen. */
void vga_blit(const uint16_t *buffer)
{
	size_t i = 0;

	while (i < VGA_WIDTH * VGA_HEIGHT)
	{
		VGA_MEMORY[i] = buffer[i];
		i++;
	}
}

/* Makes the hardware cursor visible. */
void vga_enable_cursor(void)
{
	/* Scanlines 14-15 draw the underline shape and clear bit 5, the bit that hides the cursor. */
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_START);
	outb(CRTC_DATA_PORT, 14);
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_END);
	outb(CRTC_DATA_PORT, 15);
}

/* Moves the cursor to (row, col). */
void vga_move_cursor(size_t row, size_t col)
{
	/* The registers are 8 bits wide, so the 16-bit offset goes out in two halves. */
	uint16_t offset = (uint16_t)(row * VGA_WIDTH + col);

	outb(CRTC_INDEX_PORT, CRTC_CURSOR_HIGH);
	outb(CRTC_DATA_PORT, (uint8_t)((offset >> 8) & 0xFF));
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_LOW);
	outb(CRTC_DATA_PORT, (uint8_t)(offset & 0xFF));
}
