#ifndef __BUBL_DELAY_H__
#define __BUBL_DELAY_H__

#define LOOPS_PER_USEC  80

/* Trivial udelay implementation based on cpu_speed */
static inline int udelay(int u)
{
	int ret;

	asm volatile ("0:\tsubs %0,%0,#1\n\tbpl 0b\n"
		      : "=r" (ret) : "0" (u * LOOPS_PER_USEC) );
	return ret;
}

static inline void mdelay(int m)
{
	while (m--)
		udelay(1000);
}


#endif /* __BUBL_DELAY_H__ */
