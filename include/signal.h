/* signal.h: the kernel's signal API. */

#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

/* The signals the kernel knows. Names follow Unix, numbers are our own. */
enum e_signal
{
	SIGINT,      /* Ctrl+C on the keyboard */
	SIGTRAP,     /* breakpoint or debug exception */
	SIGFPE,      /* arithmetic exception (division by zero...) */
	SIGILL,      /* invalid instruction */
	SIGSEGV,     /* memory or segment fault */
	SIGALRM,     /* raised by 'signal_schedule' when its delay expires */
	SIGUSR1,     /* free for tests */
	SIGUSR2,
	SIGNAL_COUNT
};

typedef void (*t_signal_handler)(int sig);

const char *signal_name(int sig);
int         signal_for_exception(uint32_t vector);
bool_t      signal_register(int sig, t_signal_handler handler);
bool_t      signal_raise(int sig);
bool_t      signal_schedule(int sig, uint32_t delay_ticks);
void        signal_dispatch_pending(void);

#endif
