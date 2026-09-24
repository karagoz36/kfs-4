; isr.asm: the entry points the IDT jumps to. Each vector has a tiny stub that pushes
; its number and jumps to a common routine, which saves the registers and calls C.

bits 32

section .text
extern interrupt_handler

; The CPU pushes an error code for a few exceptions only. The other vectors push a
; fake 0 so that the C side always finds the same frame (t_registers in isr.h).
%macro ISR_NO_ERROR 1
global isr%1
isr%1:
	push dword 0            ; fake error code
	push dword %1           ; vector number
	jmp isr_common
%endmacro

%macro ISR_ERROR 1
global isr%1
isr%1:
	push dword %1           ; vector number, the CPU already pushed the error code
	jmp isr_common
%endmacro

; CPU exceptions 0..31 (see the table in the subject)
ISR_NO_ERROR 0              ; division by zero
ISR_NO_ERROR 1              ; debug
ISR_NO_ERROR 2              ; non maskable interrupt
ISR_NO_ERROR 3              ; breakpoint
ISR_NO_ERROR 4              ; overflow
ISR_NO_ERROR 5              ; bound range exceeded
ISR_NO_ERROR 6              ; invalid opcode
ISR_NO_ERROR 7              ; coprocessor not available
ISR_ERROR    8              ; double fault
ISR_NO_ERROR 9              ; coprocessor segment overrun
ISR_ERROR    10             ; invalid TSS
ISR_ERROR    11             ; segment not present
ISR_ERROR    12             ; stack fault
ISR_ERROR    13             ; general protection fault
ISR_ERROR    14             ; page fault
ISR_NO_ERROR 15             ; reserved
ISR_NO_ERROR 16             ; math fault
ISR_ERROR    17             ; alignment check
ISR_NO_ERROR 18             ; machine check
ISR_NO_ERROR 19             ; SIMD floating point
ISR_NO_ERROR 20             ; 20..31 are reserved by Intel
ISR_NO_ERROR 21
ISR_NO_ERROR 22
ISR_NO_ERROR 23
ISR_NO_ERROR 24
ISR_NO_ERROR 25
ISR_NO_ERROR 26
ISR_NO_ERROR 27
ISR_NO_ERROR 28
ISR_NO_ERROR 29
ISR_NO_ERROR 30
ISR_NO_ERROR 31

; Hardware interrupts: after the PIC remap, IRQ 0..15 arrive on vectors 32..47
%assign vector 32
%rep 16
ISR_NO_ERROR vector
%assign vector vector + 1
%endrep

; Software interrupt used for system calls (bonus)
ISR_NO_ERROR 128

; Common part: build the frame, call C, undo everything and return to the interrupted code.
isr_common:
	pusha                   ; eax, ecx, edx, ebx, esp, ebp, esi, edi
	push esp                ; the argument of interrupt_handler: a pointer to that frame
	call interrupt_handler
	add esp, 4              ; drop the argument
	popa
	add esp, 8              ; drop the vector number and the error code
	iret                    ; restores eip, cs and eflags (which turns interrupts back on)

; The stubs, in vector order, so that idt.c can fill the table with a loop.
section .rodata
global isr_stub_table
isr_stub_table:
%assign vector 0
%rep 48
	dd isr %+ vector
%assign vector vector + 1
%endrep

section .note.GNU-stack noalloc noexec nowrite progbits
