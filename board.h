#include <bubl/pll.h>

extern struct pll_config *board_pll_get_config(int *adcvals);
extern int  board_boot_cfg_get_config(int *adcvals, int *adcvals_n);
extern void board_dump_config(int *adcvals_n);
extern int normalize_adc(int adcval);
extern void reset_cpu(unsigned long addr);
