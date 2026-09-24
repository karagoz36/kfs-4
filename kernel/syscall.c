/* syscall.c — the base of a system call interface (bonus). */

#include "syscall.h"
#include "signal.h"
#include "timer.h"
#include "console.h"

static int32_t sys_write(const char *buf, size_t len)
{
	size_t i = 0;

	while (i < len)
		console_putchar(buf[i++]);
	return ((int32_t)len);
}

/* Reads the number and the arguments from the saved registers, writes the result in eax: */
/* 'popa' then hands it to the caller. Without processes every call runs as the kernel. */
void syscall_handler(t_registers *regs)
{
	if (regs->eax == SYS_WRITE)
		regs->eax = (uint32_t)sys_write((const char *)regs->ebx, regs->ecx);
	else if (regs->eax == SYS_TICKS)
		regs->eax = timer_ticks();
	else if (regs->eax == SYS_KILL)
		regs->eax = signal_schedule((int)regs->ebx, 0) ? 0 : (uint32_t)-1;
	else
		regs->eax = (uint32_t)-1;
}
