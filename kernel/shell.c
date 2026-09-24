/* shell.c — a minimal debugging shell (bonus). Each command is "name" or "name argument". */

#include "shell.h"
#include "console.h"
#include "printk.h"
#include "string.h"
#include "keyboard.h"
#include "gdt.h"
#include "idt.h"
#include "stack.h"
#include "signal.h"
#include "timer.h"
#include "panic.h"
#include "syscall.h"

#define LINE_MAX 64
#define PROMPT   "kfs> "

typedef struct s_command
{
	const char *name;
	const char *help;
	void        (*run)(const char *arg);
}	t_command;

static char   g_line[LINE_MAX];
static size_t g_len = 0;

static void cmd_help(const char *arg);

static void cmd_clear(const char *arg)
{
	(void)arg;
	console_clear();
}

static void cmd_gdt(const char *arg)
{
	(void)arg;
	gdt_print();
}

static void cmd_idt(const char *arg)
{
	(void)arg;
	idt_print();
}

static void cmd_stack(const char *arg)
{
	(void)arg;
	print_kernel_stack();
}

static void cmd_uptime(const char *arg)
{
	uint32_t ticks = timer_ticks();

	(void)arg;
	printk("%u ticks = %u.%02u s (IRQ 0 at %u Hz)\n", ticks, ticks / TIMER_HZ,
		ticks % TIMER_HZ, (uint32_t)TIMER_HZ);
}

static void cmd_layout(const char *arg)
{
	if (*arg == '\0')
		printk("current layout: %s (try 'layout azerty')\n", keyboard_layout_name());
	else if (keyboard_set_layout(arg))
		printk("layout set to %s\n", arg);
	else
		printk("unknown layout '%s': qwerty or azerty\n", arg);
}

/* get_line demo (bonus): the shell is paused until Enter is pressed. */
static void cmd_read(const char *arg)
{
	char   line[LINE_MAX];
	size_t len;

	(void)arg;
	printk("type a line: ");
	len = keyboard_get_line(line, sizeof(line));
	printk("you typed '%s' (%u characters)\n", line, (uint32_t)len);
}

/* --- Signals --- */

static void on_usr1(int sig)
{
	printk("  -> callback for %s called at tick %u\n", signal_name(sig), timer_ticks());
}

/* Ctrl+C: drop the line being typed, like a terminal does. */
static void on_sigint(int sig)
{
	(void)sig;
	printk("^C\n");
	g_len = 0;
	printk(PROMPT);
}

static void cmd_signal(const char *arg)
{
	(void)arg;
	printk("register SIGUSR1 callback, raise it now, then schedule it in 1 s and 2 s\n");
	signal_register(SIGUSR1, on_usr1);
	signal_raise(SIGUSR1);
	signal_schedule(SIGUSR1, TIMER_HZ);
	signal_schedule(SIGUSR1, 2 * TIMER_HZ);
	printk("SIGUSR2 has no handler: scheduling it shows what happens then\n");
	signal_schedule(SIGUSR2, 0);
}

/* --- Interrupts: each command raises a real CPU exception --- */

static void cmd_int3(const char *arg)
{
	(void)arg;
	__asm__ volatile("int3");
	printk("back from the breakpoint: traps resume\n");
}

static void cmd_div0(const char *arg)
{
	(void)arg;
	__asm__ volatile("xor %%ecx, %%ecx\n\t"
		"mov $1, %%eax\n\t"
		"div %%ecx" : : : "eax", "ecx", "edx");
}

static void cmd_ud2(const char *arg)
{
	(void)arg;
	__asm__ volatile("ud2");
}

/* 'int 0x80' twice: a write, then a number that does not exist. */
static void cmd_syscall(const char *arg)
{
	const char *text = "hello from int 0x80\n";
	int32_t     ret;

	(void)arg;
	ret = syscall(SYS_WRITE, (uint32_t)text, k_strlen(text));
	printk("SYS_WRITE returned %d\n", ret);
	ret = syscall(SYS_TICKS, 0, 0);
	printk("SYS_TICKS returned %d\n", ret);
	ret = syscall(42, 0, 0);
	printk("syscall 42 returned %d\n", ret);
}

static void cmd_panic(const char *arg)
{
	(void)arg;
	panic("requested from the shell");
}

static void cmd_reboot(const char *arg)
{
	(void)arg;
	keyboard_reboot();
}

static void cmd_halt(const char *arg)
{
	(void)arg;
	printk("System halted.\n");
	cpu_halt_clean();
}

static const t_command g_commands[] = {
	{"help",    "list the commands",                   cmd_help},
	{"clear",   "clear the screen",                    cmd_clear},
	{"gdt",     "print the GDT",                       cmd_gdt},
	{"idt",     "print the IDT",                       cmd_idt},
	{"stack",   "print the kernel stack",              cmd_stack},
	{"uptime",  "ticks of the timer (IRQ 0)",          cmd_uptime},
	{"layout",  "show or set the keyboard layout",     cmd_layout},
	{"read",    "read a line with get_line",           cmd_read},
	{"signal",  "signal callbacks and scheduling",     cmd_signal},
	{"int3",    "breakpoint exception (resumes)",      cmd_int3},
	{"div0",    "division by zero (panics)",           cmd_div0},
	{"ud2",     "invalid opcode (panics)",             cmd_ud2},
	{"syscall", "call the kernel through int 0x80",    cmd_syscall},
	{"panic",   "kernel panic on purpose",             cmd_panic},
	{"reboot",  "restart the machine",                 cmd_reboot},
	{"halt",    "clear the registers and stop",        cmd_halt},
};
#define COMMAND_COUNT (sizeof(g_commands) / sizeof(g_commands[0]))

static void cmd_help(const char *arg)
{
	size_t i = 0;
	size_t pad;

	(void)arg;
	while (i < COMMAND_COUNT)
	{
		printk("  %s", g_commands[i].name);
		pad = k_strlen(g_commands[i].name);
		while (pad++ < 9)
			console_putchar(' ');
		printk("%s\n", g_commands[i].help);
		i++;
	}
}

/* Runs the typed line: the first word is the command, the rest is its argument. */
static void run_line(void)
{
	char   *cmd = g_line;
	char   *arg;
	size_t  i = 0;

	g_line[g_len] = '\0';
	while (*cmd == ' ')
		cmd++;
	if (*cmd == '\0')
		return ;
	arg = cmd;
	while (*arg != '\0' && *arg != ' ')
		arg++;
	if (*arg == ' ')
		*arg++ = '\0';
	while (*arg == ' ')
		arg++;
	while (i < COMMAND_COUNT)
	{
		if (k_strcmp(cmd, g_commands[i].name) == 0)
		{
			g_commands[i].run(arg);
			return ;
		}
		i++;
	}
	printk("unknown command: '%s' (try 'help')\n", cmd);
}

void shell_init(void)
{
	signal_register(SIGINT, on_sigint);
	printk(PROMPT);
}

/* Called for every character typed. */
void shell_input(char c)
{
	if (c == '\n')
	{
		console_putchar('\n');
		run_line();
		g_len = 0;
		printk(PROMPT);
	}
	else if (c == '\b')
	{
		if (g_len > 0)
		{
			g_len--;
			console_putchar('\b');
		}
	}
	else if ((uint8_t)c >= ' ' && g_len < LINE_MAX - 1)
	{
		g_line[g_len++] = c;
		console_putchar(c);
	}
}
