#ifndef __BUBL_DELAY_H__
#define __BUBL_DELAY_H__
#include <bubl/timer.h>

/* Trivial udelay implementation based on timer 1 */
static inline int udelay(int u)
{
	u32 count = timer_read();

	while (count_to_usec(timer_read() - count) < 1000)
		/* wait */;
	return 0;
}

static inline void mdelay(int m)
{
	while (m--)
		udelay(1000);
}


#endif /* __BUBL_DELAY_H__ */
