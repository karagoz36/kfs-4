/* keyboard.c — PS/2 keyboard driver. IRQ 1 decodes the scancode and drops the character */
/* in a small queue; the main loop and 'keyboard_get_line' read from that queue. */

#include "keyboard.h"
#include "console.h"
#include "signal.h"
#include "string.h"
#include "isr.h"
#include "io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_RESET   0xFE  /* controller command: pulse the CPU reset line */

#define SC_RELEASE_FLAG 0x80  /* the bit that marks break codes */
#define SC_EXTENDED     0xE0  /* some keys send two bytes */

#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CTRL         0x1D
#define SC_ALT          0x38
#define SC_DIGIT1       0x02  /* 1..4 -> 0x02, 0x03, 0x04, 0x05 */
#define SC_C            0x2E  /* Ctrl+C */

#define QUEUE_SIZE 64

/* Current state of the modifier keys */
static bool_t g_shift = FALSE;
static bool_t g_ctrl  = FALSE;
static bool_t g_alt   = FALSE;

/* Whether the next scancode follows an 0xE0 (extended) prefix */
static bool_t g_extended = FALSE;

/* Ring of characters typed but not read yet. The IRQ writes at 'head', the main loop */
/* reads at 'tail'; both only move forward, so no lock is needed. */
static volatile char   g_queue[QUEUE_SIZE];
static volatile size_t g_head = 0;
static volatile size_t g_tail = 0;

/* A layout: what a scancode gives without and with shift (bonus: several layouts). */
/* Accented letters use their code page 437 value, which is what the VGA font draws. */
typedef struct s_layout
{
	const char *name;
	uint8_t     plain[SC_RELEASE_FLAG];
	uint8_t     shift[SC_RELEASE_FLAG];
}	t_layout;

static const t_layout g_layouts[] = {
	{"qwerty",
	{0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	 '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	 0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	 0,    '*', 0,   ' '},
	{0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	 '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	 0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	 0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	 0,    '*', 0,   ' '}},
	{"azerty",
	/* 0x82 e-acute, 0x8A e-grave, 0x87 c-cedilla, 0x85 a-grave, 0x97 u-grave, 0xFD superscript 2 */
	{0,    27,  '&', 0x82, '"', '\'', '(', '-', 0x8A, '_', 0x87, 0x85, ')', '=', '\b',
	 '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
	 0,    'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 0x97, 0xFD,
	 0,    '*', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!',
	 0,    '*', 0,   ' ', [0x56] = '<'},
	/* 0xF8 degree sign, 0x9C pound sign, 0xE6 mu, 0x15 section sign */
	{0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0xF8, '+', '\b',
	 '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '"', 0x9C, '\n',
	 0,    'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', 0,
	 0,    0xE6, 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', 0x15,
	 0,    '*', 0,   ' ', [0x56] = '>'}},
};
#define LAYOUT_COUNT (sizeof(g_layouts) / sizeof(g_layouts[0]))

static const t_layout *g_layout = &g_layouts[0];

static void queue_push(char c)
{
	size_t next = (g_head + 1) % QUEUE_SIZE;

	if (next == g_tail)  /* full: the character is lost, like a real keyboard buffer */
		return ;
	g_queue[g_head] = c;
	g_head = next;
}

/* Handles modifier keys; returns TRUE when it did (no character is produced). */
static bool_t handle_modifier(uint8_t code, bool_t pressed)
{
	if (code == SC_LSHIFT || code == SC_RSHIFT)
		g_shift = pressed;
	else if (code == SC_CTRL)
		g_ctrl = pressed;
	else if (code == SC_ALT)
		g_alt = pressed;
	else
		return (FALSE);
	return (TRUE);
}

/* Shortcuts: Alt+1..4 switches screen, Ctrl+C raises SIGINT. Returns TRUE when it did. */
static bool_t handle_shortcut(uint8_t code)
{
	if (g_ctrl && code == SC_C)
	{
		signal_schedule(SIGINT, 0);
		return (TRUE);
	}
	if (!g_alt && !g_ctrl)
		return (FALSE);
	if (code < SC_DIGIT1 || code >= SC_DIGIT1 + CONSOLE_COUNT)
		return (FALSE);
	console_switch((size_t)(code - SC_DIGIT1));
	return (TRUE);
}

/* Turns one scancode into a character, or 0 when there is none to produce. */
static char decode(uint8_t scancode)
{
	bool_t  pressed;
	uint8_t code;

	/* 0xE0 is a prefix: the real code arrives with the next byte */
	if (scancode == SC_EXTENDED)
	{
		g_extended = TRUE;
		return (0);
	}
	pressed = (scancode & SC_RELEASE_FLAG) ? FALSE : TRUE;
	code = scancode & (uint8_t)(SC_RELEASE_FLAG - 1);
	if (handle_modifier(code, pressed) || !pressed || g_extended)
	{
		g_extended = FALSE;
		return (0);
	}
	if (handle_shortcut(code))
		return (0);
	return ((char)(g_shift ? g_layout->shift[code] : g_layout->plain[code]));
}

/* IRQ 1: the controller has a byte for us. It MUST be read, or the keyboard stays silent. */
static void keyboard_irq(t_registers *regs)
{
	char c;

	(void)regs;
	c = decode(inb(PS2_DATA_PORT));
	if (c != 0)
		queue_push(c);
}

void keyboard_init(void)
{
	irq_register(IRQ_KEYBOARD, keyboard_irq);
}

/* Next character typed, or 0 when the queue is empty. Never blocks. */
char keyboard_getchar(void)
{
	char c;

	if (g_head == g_tail)
		return (0);
	c = g_queue[g_tail];
	g_tail = (g_tail + 1) % QUEUE_SIZE;
	return (c);
}

/* Like read() on a terminal (bonus): echoes what is typed, handles backspace, and */
/* returns the line (without the '\n') once Enter is pressed. The CPU sleeps in between. */
size_t keyboard_get_line(char *buf, size_t size)
{
	size_t len = 0;
	char   c;

	while (TRUE)
	{
		c = keyboard_getchar();
		if (c == 0)
			__asm__ volatile("hlt");
		else if (c == '\n')
			break ;
		else if (c == '\b')
		{
			if (len > 0)
			{
				len--;
				console_putchar('\b');
			}
		}
		else if (len + 1 < size)
		{
			buf[len++] = c;
			console_putchar(c);
		}
	}
	console_putchar('\n');
	buf[len] = '\0';
	return (len);
}

bool_t keyboard_set_layout(const char *name)
{
	size_t i = 0;

	while (i < LAYOUT_COUNT)
	{
		if (k_strcmp(name, g_layouts[i].name) == 0)
		{
			g_layout = &g_layouts[i];
			return (TRUE);
		}
		i++;
	}
	return (FALSE);
}

const char *keyboard_layout_name(void)
{
	return (g_layout->name);
}

/* Reboot: the keyboard controller can pulse the CPU's reset line. */
void keyboard_reboot(void)
{
	outb(PS2_STATUS_PORT, PS2_CMD_RESET);
}
