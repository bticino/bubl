#include <bubl/pll.h>

extern struct pll_config *board_get_config(int *adcvals);
extern void board_dump_config(int *adcvals);
extern int normalize_adc(int adcval);

