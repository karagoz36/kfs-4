/* printk.c — printf-like printing function. */

#include "printk.h"
#include "console.h"
#include "string.h"

typedef __builtin_va_list va_list_t;
#define VA_START(ap, last) __builtin_va_start(ap, last)
#define VA_ARG(ap, type)   __builtin_va_arg(ap, type)
#define VA_END(ap)         __builtin_va_end(ap)

/* Prints an unsigned number in base 10 or 16, left-padded with zeros to 'width' digits (0 = no padding). */
static void print_uint(uint32_t value, uint32_t base, size_t width)
{
	const char *digits = "0123456789abcdef";
	char        tmp[32];
	size_t      len = 0;

	/* value % base yields the rightmost digit first, so the digits are collected in reverse. */
	if (value == 0)
		tmp[len++] = '0';
	while (value > 0)
	{
		tmp[len++] = digits[value % base];
		value /= base;
	}
	while (len < width)
		tmp[len++] = '0';
	while (len > 0)
		console_putchar(tmp[--len]);
}

/* Signed number: print '-' first, then the magnitude. */
static void print_int(int32_t value)
{
	uint32_t magnitude;

	if (value < 0)
	{
		console_putchar('-');
		/* -value overflows for INT_MIN, so the sign is flipped with unsigned arithmetic instead. */
		magnitude = (uint32_t)(~(uint32_t)value + 1u);
	}
	else
		magnitude = (uint32_t)value;
	print_uint(magnitude, 10, 0);
}

void printk(const char *format, ...)
{
	va_list_t ap;
	size_t    i = 0;
	size_t    width;

	VA_START(ap, format);
	while (format[i] != '\0')
	{
		/* Ordinary characters go straight to the screen */
		if (format[i] != '%')
		{
			console_putchar(format[i++]);
			continue ;
		}
		i++;
		/* Optional zero padding: "%08x" prints 8 hex digits, used to align the dump columns. */
		width = 0;
		if (format[i] == '0')
		{
			i++;
			while (format[i] >= '0' && format[i] <= '9')
				width = width * 10 + (size_t)(format[i++] - '0');
		}
		/* char is promoted to int through '...', so it is read back as an int and narrowed. */
		if (format[i] == 'c')
			console_putchar((char)VA_ARG(ap, int));
		else if (format[i] == 's')
		{
			const char *s = VA_ARG(ap, const char *);

			console_write(s ? s : "(null)");
		}
		else if (format[i] == 'd' || format[i] == 'i')
			print_int(VA_ARG(ap, int32_t));
		else if (format[i] == 'u')
			print_uint(VA_ARG(ap, uint32_t), 10, width);
		else if (format[i] == 'x')
			print_uint(VA_ARG(ap, uint32_t), 16, width);
		else if (format[i] == 'p')
		{
			/* Pointers are printed as 0x... in hexadecimal */
			console_write("0x");
			print_uint((uint32_t)VA_ARG(ap, void *), 16, width);
		}
		else if (format[i] == '%')
			console_putchar('%');
		else
		{
			/* Unknown specifier: print it as it was written (%q -> "%q") */
			console_putchar('%');
			if (format[i] != '\0')
				console_putchar(format[i]);
		}
		if (format[i] != '\0')
			i++;
	}
	VA_END(ap);
}
