#ifndef __BUBL_TOOLS_H__
#define __BUBL_TOOLS_H__
#include <stdarg.h>

extern unsigned long simple_strtoul(const char *cp,char **endp,
				    unsigned int base);
extern long simple_strtol(const char *cp,char **endp,unsigned int base);
extern int ustrtoul(const char *cp, char **endp, unsigned int base);

#define NULL 0

extern int sprintf(char * buf, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
extern int vsprintf(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));
extern int vsprintf_full(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));

extern int vprintk(const char *fmt, va_list args)
	__attribute__ ((format (printf, 1, 0)));
extern int printk(const char * fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

/* Generic useful macros from Linux */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#endif /* __BUBL_TOOLS_H__ */
