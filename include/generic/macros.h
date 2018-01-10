#ifndef __MACRO_H
#define __MACRO_H

/* Borrowed from the linux kernel. Don't try to understand it. */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#ifndef CONFIG_TOOLCHAIN_SDCC
#define min_t(type, x, y) ({					\
			type __min1 = (x);			\
			type __min2 = (y);			\
			__min1 < __min2 ? __min1: __min2; })


#define max_t(type, x, y) ({					\
			type __max1 = (x);			\
			type __max2 = (y);			\
			__max1 > __max2 ? __max1: __max2; })

#else


#define min_t(type, a, b) (((type)(a)<(type)(b))?(type)(a):(type)(b))
#define max_t(type, a, b) (((type)(a)>(type)(b))?(type)(a):(type)(b))

#endif

/* Just in case */
#ifndef NULL
#define NULL 0
#endif

/* Macro concatenation magic.  
 * Stolen from somewhere on the internets
 */

/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define PPCAT_NX(A, B) A ## B

/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define PPCAT_NX3(A, B, C) A ## B ## C

/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT(A, B) PPCAT_NX(A, B)

/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT3(A, B, C) PPCAT_NX3(A, B, C)


#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

/* 
 * Macrofied, so that we can later add implementations
 * for compilers that don't know about 0b prefix 
 * So far both gcc and sdcc can do this stuff
 */

#define BIN(x) 0b##x

#ifndef _BV
#define _BV(a) (1<<a)
#endif

#endif

