#ifndef _MMCSD_H
#define _MMCSD_H

#include <bubl/types.h>

/* These initial functions are in mmc_glue.c */
extern int sdmmc_init(void);
extern int sdmmc_read_blocks(int dev, int start_blk, int num_blks, int unsigned long addr);

#endif
