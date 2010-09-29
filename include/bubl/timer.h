/*
 * Register definitions for the timer 1
 */
#include <bubl/types.h>

enum timer_regs {
	TIM_PID12,
	TIM_EMUMGT,
	TIM_UNUSED_08,
	TIM_UNUSED_0c,

	TIM_TIM12,
	TIM_TIM34,
	TIM_PRD12,
	TIM_PRD34,

	TIM_TCR,
	TIM_TGCR,
	TIM_WDTCR,
	TIM_UNUSED_2c,

	TIM_UNUSED_30,
	TIM_REL12,
	TIM_REL34,
	TIM_CAP12,

	TIM_CAP34,
	TIM_INTCTL_STAT
};

extern void timer_setup(void);
extern u32 timer_read(void);
extern u32 count_to_usec(u32 count);
extern int nop(int nloop);
