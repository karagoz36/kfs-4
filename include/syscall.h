/* syscall.h: system calls through 'int 0x80' (bonus). */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

#define SYSCALL_VECTOR 0x80

/* The number goes in eax, the arguments in ebx and ecx, the result comes back in eax. */
enum e_syscall
{
	SYS_WRITE,   /* (const char *buf, size_t len) -> len */
	SYS_TICKS,   /* () -> timer ticks since boot */
	SYS_KILL,    /* (int sig) -> 0, or -1 for an unknown signal */
	SYSCALL_COUNT
};

void syscall_handler(t_registers *regs);

/* The user side: what a libc would wrap in write(), time()... */
static inline int32_t syscall(uint32_t number, uint32_t arg1, uint32_t arg2)
{
	int32_t result;

	__asm__ volatile("int $0x80"
		: "=a"(result)
		: "a"(number), "b"(arg1), "c"(arg2)
		: "memory");
	return (result);
}

#endif
