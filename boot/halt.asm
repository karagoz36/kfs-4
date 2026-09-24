; halt.asm: the last thing a panic or the 'halt' command runs. void cpu_halt_clean(void);

bits 32

section .text
global cpu_halt_clean

; Interrupts off, every general register cleared, then the CPU sleeps forever.
; esp is left alone: there is nothing to return to, but 'hlt' needs a valid stack for NMIs.
cpu_halt_clean:
	cli
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	xor esi, esi
	xor edi, edi
	xor ebp, ebp
.hang:
	hlt
	jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
