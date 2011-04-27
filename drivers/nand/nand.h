/*
 * (C) Copyright 2005
 * 2N Telekomunikace, a.s. <www.2n.cz>
 * Ladislav Michl <michl@2n.cz>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _NAND_H_
#define _NAND_H_


extern int nand_init(void);

#include "mtd/compat.h"
#include "mtd/mtd.h"
#include "mtd/nand.h"
#include <bubl/tools.h>

#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define __arch_putb(v, a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v, a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v, a)		(*(volatile unsigned int *)(a) = (v))

#define __raw_writeb(v, a)		__arch_putb(v, a)
#define __raw_writew(v, a)		__arch_putw(v, a)
#define __raw_writel(v, a)		__arch_putl(v, a)

#define __raw_readb(a)			__arch_getb(a)
#define __raw_readw(a)			__arch_getw(a)
#define __raw_readl(a)			__arch_getl(a)

#define writeb(v, a)			__arch_putb(v, a)
#define writew(v, a)			__arch_putw(v, a)
#define writel(v, a)			__arch_putl(v, a)

#define readb(a)			__arch_getb(a)
#define readw(a)			__arch_getw(a)
#define readl(a)			__arch_getl(a)

extern int board_nand_init(struct nand_chip *nand);

typedef struct mtd_info nand_info_t;

extern int nand_curr_device;

int nand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
		int8_t *buf);
int nand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
		const uint8_t *buf);
int nand_block_isbad(nand_info_t *info, loff_t ofs);
int nand_erase(nand_info_t *info, loff_t off, size_t size);

int read_uboot_flash(struct mtd_info *mtd, int start_page, unsigned char *start);


/*****************************************************************************
 * declarations from nand_util.c
 ****************************************************************************/

struct nand_write_options {
	u_char *buffer;		/* memory block containing image to write */
	ulong length;		/* number of bytes to write */
	ulong offset;		/* start address in NAND */
	int quiet;		/* don't display progress messages */
	int autoplace;		/* if true use auto oob layout */
#if 0
	int forcejffs2;		/* force jffs2 oob layout */
	int forceyaffs;		/* force yaffs oob layout */
#endif
	int noecc;		/* write without ecc */
	int writeoob;		/* image contains oob data */
	int pad;		/* pad to page size */
	int blockalign;		/* 1|2|4 set multiple of eraseblocks
				 * to align to */
};

typedef struct nand_write_options nand_write_options_t;
typedef struct mtd_oob_ops mtd_oob_ops_t;

struct nand_read_options {
	u_char *buffer;		/* memory block in which read image is written*/
	ulong length;		/* number of bytes to read */
	ulong offset;		/* start address in NAND */
	int quiet;		/* don't display progress messages */
	int readoob;		/* put oob data in image */
};

typedef struct nand_read_options nand_read_options_t;

struct nand_erase_options {
	loff_t length;		/* number of bytes to erase */
	loff_t offset;		/* first address in NAND to erase */
	int quiet;		/* don't display progress messages */
#if 0
	int jffs2;		/* if true: format for jffs2 usage
				 * (write appropriate cleanmarker blocks) */
#endif
	int scrub;		/* if true, really clean NAND by erasing
				 * bad blocks (UNSAFE) */

	/* Don't include skipped bad blocks in size to be erased */
	int spread;
};

typedef struct nand_erase_options nand_erase_options_t;

int nand_read_skip_bad(nand_info_t *nand, loff_t offset, size_t *length,
		       u_char *buffer);
int nand_write_skip_bad(nand_info_t *nand, loff_t offset, size_t *length,
			u_char *buffer);
int nand_erase_opts(nand_info_t *meminfo, const nand_erase_options_t *opts);

#define NAND_LOCK_STATUS_TIGHT	0x01
#define NAND_LOCK_STATUS_LOCK	0x02
#define NAND_LOCK_STATUS_UNLOCK 0x04

int nand_lock(nand_info_t *meminfo, int tight);
int nand_unlock(nand_info_t *meminfo, ulong start, ulong length);
int nand_get_lock_status(nand_info_t *meminfo, loff_t offset);

#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
void board_nand_select_device(struct nand_chip *nand, int chip);
#endif

__attribute__((noreturn)) void nand_boot(void);

#endif

#ifdef CONFIG_ENV_OFFSET_OOB
#define ENV_OOB_MARKER 0x30425645 /*"EVB0" in little-endian -- offset is stored
				    as block number*/
#define ENV_OOB_MARKER_OLD 0x30564e45 /*"ENV0" in little-endian -- offset is
					stored as byte number */
#define ENV_OFFSET_SIZE 8
int get_nand_env_oob(nand_info_t *nand, unsigned long *result);
#endif
