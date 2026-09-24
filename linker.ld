/* linker.ld: our own layout; the host's .ld file is forbidden and would not boot anyway. */

/* The entry point the bootloader jumps to (defined in boot.asm) */
ENTRY(_start)

SECTIONS
{
	/* GRUB loads nothing below 1 MB, so this is only a symbol; kernel/gdt.c fills the table. */
	gdt_table = 0x00000800;

	/* The kernel goes at 1 MB: the first megabyte holds BIOS, VGA and device regions. */
	. = 1M;

	/* The multiboot header must sit within the first 8 KB -> put it first */
	.text ALIGN(4K) :
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data (string literals, keymap tables) */
	.rodata ALIGN(4K) :
	{
		*(.rodata)
	}

	/* Initialised global/static variables */
	.data ALIGN(4K) :
	{
		*(.data)
	}

	/* Zeroed variables and the stack: reserved at run time, no space in the binary */
	.bss ALIGN(4K) :
	{
		*(COMMON)
		*(.bss)
	}
}
