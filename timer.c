/*
 * Timer support.
 *
 * This is used to check the clock speed by watching a free-running timer
 */
#include <bubl/types.h>
#include <bubl/timer.h>

#define timer_base ((volatile u32 *)0x01c21800) /* Timer 1 (0 used by uboot) */

void timer_setup(void)
{
	timer_base[TIM_EMUMGT] = 1; /* Free running */
	timer_base[TIM_TCR] = (2 << 22); /* Continuous 34 */
	timer_base[TIM_PRD12] = ~0;
	timer_base[TIM_PRD34] = ~0; /* FIXME */
	timer_base[TIM_TGCR] = 0
		| (3 << 8) /* prescaler34: 24MHz -> 8MHz */
		| (1 << 4) /* new features? */
		| (1 << 2) /* duea 32 bit unchained */
		| 3; /* both out of reset; */
}

u32 timer_read(void)
{
	return timer_base[TIM_TIM34];
}

/* Return value is usecs. Timer frequency is 24MHz / 3 , hardwired */
u32 count_to_usec(u32 count)

{
	return count / 8;
}

/* Run nop operation for nloop times, return usecs */
int nop(int nloop)
{
	u32 count;
	int i;

	nloop = (nloop + 128) / 256;
	count = timer_read();
	for (i = 0; i < nloop; i++)
	    asm volatile(".rep 256\n\tnop\n.endr");
	count = timer_read() - count;
	return count_to_usec(count);
}
