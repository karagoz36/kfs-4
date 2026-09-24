/* signal.c — signals: a number, a callback, and a way to deliver it now or later. */

#include "signal.h"
#include "timer.h"
#include "printk.h"

#define PENDING_MAX 16

/* A signal waiting for its time to come */
typedef struct s_pending
{
	bool_t   used;
	int      sig;
	uint32_t due_tick;   /* delivered once timer_ticks() reaches it */
}	t_pending;

static const char *g_names[SIGNAL_COUNT] = {
	"SIGINT", "SIGTRAP", "SIGFPE", "SIGILL", "SIGSEGV", "SIGALRM", "SIGUSR1", "SIGUSR2"
};

static t_signal_handler g_handlers[SIGNAL_COUNT];
static t_pending        g_pending[PENDING_MAX];

const char *signal_name(int sig)
{
	if (sig < 0 || sig >= SIGNAL_COUNT)
		return ("?");
	return (g_names[sig]);
}

/* Which signal a CPU exception maps to; -1 for the ones no callback can help with. */
int signal_for_exception(uint32_t vector)
{
	if (vector == 0 || vector == 16 || vector == 19)
		return (SIGFPE);
	if (vector == 1 || vector == 3 || vector == 4)
		return (SIGTRAP);
	if (vector == 6 || vector == 7)
		return (SIGILL);
	if (vector == 5 || (vector >= 10 && vector <= 14) || vector == 17)
		return (SIGSEGV);
	return (-1);
}

/* Installs (or removes, with NULL) the callback of a signal. */
bool_t signal_register(int sig, t_signal_handler handler)
{
	if (sig < 0 || sig >= SIGNAL_COUNT)
		return (FALSE);
	g_handlers[sig] = handler;
	return (TRUE);
}

/* Delivers a signal right now. Returns FALSE when nobody handles it. */
bool_t signal_raise(int sig)
{
	if (sig < 0 || sig >= SIGNAL_COUNT || g_handlers[sig] == NULL)
		return (FALSE);
	g_handlers[sig](sig);
	return (TRUE);
}

/* Queues a signal for later: 0 ticks means "at the next pass of the main loop". */
/* The shell and the keyboard IRQ may both call this, so interrupts are paused while a */
/* free slot is taken; pushf/popf put them back the way they were (off inside an IRQ). */
bool_t signal_schedule(int sig, uint32_t delay_ticks)
{
	uint32_t flags;
	size_t   i = 0;
	bool_t   queued = FALSE;

	if (sig < 0 || sig >= SIGNAL_COUNT)
		return (FALSE);
	__asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags));
	while (i < PENDING_MAX && !queued)
	{
		if (!g_pending[i].used)
		{
			g_pending[i].sig = sig;
			g_pending[i].due_tick = timer_ticks() + delay_ticks;
			g_pending[i].used = TRUE;
			queued = TRUE;
		}
		i++;
	}
	__asm__ volatile("push %0\n\tpopf" : : "r"(flags));
	return (queued);
}

/* Called by the main loop, outside of any interrupt: delivers what is due. */
/* Only this function clears a slot and only signal_schedule fills one, so no locking. */
void signal_dispatch_pending(void)
{
	size_t i = 0;
	int    sig;

	while (i < PENDING_MAX)
	{
		if (g_pending[i].used && timer_ticks() >= g_pending[i].due_tick)
		{
			sig = g_pending[i].sig;
			g_pending[i].used = FALSE;
			if (!signal_raise(sig))
				printk("signal %s: no handler, ignored\n", signal_name(sig));
		}
		i++;
	}
}
