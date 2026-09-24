/* vga.h: text mode 80x25 at 0xB8000; each cell is 2 bytes, character then color attribute. */

#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
/* 16-bit cells indexed by row * VGA_WIDTH + col; 'volatile' because this is a device, not RAM. */
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

/* The standard 16-color VGA palette (bonus: color support) */
enum e_vga_color
{
	VGA_BLACK = 0,
	VGA_BLUE,
	VGA_GREEN,
	VGA_CYAN,
	VGA_RED,
	VGA_MAGENTA,
	VGA_BROWN,
	VGA_LIGHT_GREY,
	VGA_DARK_GREY,
	VGA_LIGHT_BLUE,
	VGA_LIGHT_GREEN,
	VGA_LIGHT_CYAN,
	VGA_LIGHT_RED,
	VGA_LIGHT_MAGENTA,
	VGA_YELLOW,
	VGA_WHITE
};

/* Attribute byte: bg in the high nibble, fg in the low one. fg 11, bg 0 -> 0x0B */
static inline uint8_t vga_color(uint8_t fg, uint8_t bg)
{
	return ((uint8_t)(fg | (bg << 4)));
}

/* Cell: character in the low byte, attribute in the high one. '4' with 0x0B -> 0x0B34 */
static inline uint16_t vga_entry(char c, uint8_t color)
{
	return ((uint16_t)(uint8_t)c | ((uint16_t)color << 8));
}

void vga_write_cell(size_t index, uint16_t cell);
void vga_blit(const uint16_t *buffer);
void vga_enable_cursor(void);
void vga_move_cursor(size_t row, size_t col);

#endif
