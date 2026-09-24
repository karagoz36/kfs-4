; gdt_flush.asm: runs lgdt and reloads every segment register. void gdt_flush(const t_gdt_ptr *);

bits 32

KERNEL_CODE  equ 0x08   ; entry 1
KERNEL_DATA  equ 0x10   ; entry 2
KERNEL_STACK equ 0x18   ; entry 3

section .text
global gdt_flush

gdt_flush:
	mov eax, [esp + 4]      ; first argument: address of the 6-byte GDT pointer
	lgdt [eax]              ; "declare" the table to the CPU

	jmp KERNEL_CODE:.reload_segments   ; CS takes no 'mov': a far jump reloads it

.reload_segments:
	mov ax, KERNEL_DATA
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ax, KERNEL_STACK
	mov ss, ax              ; SS uses its own entry (kernel stack)
	ret                     ; base 0 everywhere, so the return address is still valid

section .note.GNU-stack noalloc noexec nowrite progbits
