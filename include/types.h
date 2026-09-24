/* types.h: the kernel is freestanding (-nostdlib), so <stdint.h> is unavailable. */

#ifndef TYPES_H
#define TYPES_H

/* On i386 the sizes are fixed: char 1, short 2, int 4 bytes. */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;

/* A pointer is 4 bytes on i386, hence size_t = unsigned int */
typedef unsigned int       size_t;

/* Minimal boolean; C99 <stdbool.h> is not available */
typedef uint8_t            bool_t;
#define TRUE  1
#define FALSE 0

#define NULL ((void *)0)

#endif
