; boot.asm: multiboot header, 16 KB stack, and the entry point that calls kernel_main.

bits 32

; Multiboot v1 header: GRUB scans the first 8 KB for MAGIC; FLAGS = 0 asks for nothing extra.
MB_MAGIC    equ 0x1BADB002               ; multiboot 1 signature
MB_FLAGS    equ 0
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)   ; magic + flags + checksum must be 0

; Own section: the linker script puts it first so GRUB is guaranteed to find it.
section .multiboot
align 4
	dd MB_MAGIC
	dd MB_FLAGS
	dd MB_CHECKSUM

; Stack: 16 KB in .bss, 16-byte aligned; both ends are global so the C side can print it.
section .bss
align 16
global stack_bottom
global stack_top
stack_bottom:
	resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
	mov esp, stack_top      ; the stack grows downwards, so esp starts at the top
	xor ebp, ebp            ; zero frame pointer: marks the end of the backtrace chain

	call kernel_main        ; hand over to C; this call is not expected to return

	cli                     ; should it return anyway: halt forever
.hang:
	hlt
	jmp .hang

; Tells modern linkers the stack need not be executable; without it ld warns.
section .note.GNU-stack noalloc noexec nowrite progbits
