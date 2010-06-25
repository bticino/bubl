/*
 *  From: linux/kernel/printk.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Shrunk and de-featured: Alessandro Rubini 2010
 */

#include <stdarg.h>
#include <bubl/io.h>
#include <bubl/tools.h>

#ifndef BUBL_PK_BUF
#define BUBL_PK_BUF	256
#endif

static char printk_buf[BUBL_PK_BUF];

int vprintk(const char *fmt, va_list args)
{
	int ret;

	ret = vsprintf(printk_buf, fmt, args);
	puts(printk_buf);
	return ret;
}

int printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}
