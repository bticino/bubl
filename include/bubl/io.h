#ifndef __BLUB_IO_H__
#define __BLUB_IO_H__

extern void putc(int c);
extern void puts(const char *s);
extern int getc(void);
extern int testc(void);
extern char *gets(char *s, int len);
extern void serial_setup(void);

#endif /* __TSW_IO_H__ */
