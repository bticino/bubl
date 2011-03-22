/*
 * Timer support.
 *
 * This is used to check the clock speed by watching a free-running timer
 */
#include <bubl/types.h>
#include <bubl/timer.h>

#define CONFIG_SYS_HZ_CLOCK		24000000
#define CONFIG_SYS_WATCHDOG_VALUE	(5 * 60) /* seconds to watchdog reboot */

#define timer_base ((volatile u32 *)0x01c21800) /* Timer 1 (0 used by uboot) */
#define wdtimer_base ((volatile u32 *)0x01c21c00) /* Timer 2 (also used by uboot) */

static void wd_timer_init(void)
{
	uint64_t timeo = ((uint64_t) CONFIG_SYS_HZ_CLOCK) *
				((uint64_t) CONFIG_SYS_WATCHDOG_VALUE);

	/* Disable, internal clock source */
	wdtimer_base[TIM_TCR] = 0;

	/* Reset timer, set mode to 64-bit watchdog, and unreset */
	wdtimer_base[TIM_TGCR] = 0;
	wdtimer_base[TIM_TGCR] = (2 << 2) | 3;

	/* Clear counter regs */
	wdtimer_base[TIM_TIM12] = 0;
	wdtimer_base[TIM_TIM34] = 0;

	/* Set timeout period */
	wdtimer_base[TIM_PRD12] = timeo & 0xffffffff;
	wdtimer_base[TIM_PRD34] = timeo >> 32;

	/* Enable run continuously */
	wdtimer_base[TIM_TCR] = (2 << 6) | (2 << 22);

	wdtimer_base[TIM_WDTCR] = 0xa5c64000;
	wdtimer_base[TIM_WDTCR] = 0xda7e4000;

	wdtimer_base[TIM_EMUMGT] = 0x1; /* free running */

}

void timer_setup(void)
{
	/* First of all, reset the timer */
	timer_base[TIM_TGCR] = 0;

	timer_base[TIM_EMUMGT] = 1; /* Free running */
	timer_base[TIM_TCR] = (2 << 22); /* Continuous 34 */
	timer_base[TIM_PRD12] = ~0;
	timer_base[TIM_PRD34] = ~0; /* FIXME */
	timer_base[TIM_TGCR] = 0
		| (2 << 8) /* prescaler34: 24MHz -> 8MHz */
		| (1 << 4) /* new features? */
		| (1 << 2) /* dual 32 bit unchained */
		| 3; /* both out of reset; */

	wd_timer_init();
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

/* This get_jiffies is used by Linux code. Use 1kHZ fake rate */
unsigned long get_jiffies(void)
{
	static unsigned long prevj;
	static u32 prevcount;
	static int rest_usec;

	u32 count;
	int usec;

	/* This is all approximated, who cares */
	count = timer_read();
	usec = count_to_usec(count - prevcount);
	prevcount = count;
	rest_usec += usec;
	prevj += rest_usec / USECS_PER_JIFFY;
	rest_usec %= USECS_PER_JIFFY;
	return prevj;
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

/* Run memory access for nloop times, return usecs */
int mw(int nloop, void *addr)
{
	u32 count;
	int i;

	nloop = (nloop + 128) / 256;
	count = timer_read();
	for (i = 0; i < nloop; i++)
		asm volatile(".rep 16\n\t"
			     "stm %0, {r0-r15}\n\t"
			     ".endr" : /* no output */ : "r" (addr));
	count = timer_read() - count;
	return count_to_usec(count);
}
