/*
 *  drivers/mtd/nand.c
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/doc/nand.html
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *		  2002-2006 Thomas Gleixner (tglx@linutronix.de)
 *
 *  Credits:
 *	David Woodhouse for adding multichip support
 *
 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 *  TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *	BBT table is not serialized, has to be fixed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <bubl/types.h>
#include <bubl/tools.h>
#include <bubl/timer.h>
#include <bubl/delay.h>
#include <bubl/string.h>
#include <bubl/errno.h>
#include "mtd/compat.h"
#include "mtd/mtd.h"
#include "mtd/nand.h"

#ifndef CONFIG_SYS_NAND_MAX_CHIPS
#define CONFIG_SYS_NAND_MAX_CHIPS       1
#endif
#define U_BOOT_COPIES			4
#define U_BOOT_MAX_DIM			0x60000

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

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define CONFIG_SYS_HZ 1000

#undef VERBOSE
#undef FLASH16BIT

#define min(X, Y)                               \
	({ typeof(X) __x = (X), __y = (Y);     \
		(__x < __y) ? __x : __y; })

#define max(X, Y)                               \
	({ typeof(X) __x = (X), __y = (Y);     \
		(__x > __y) ? __x : __y; })

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)

int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/*
 * CONFIG_SYS_NAND_RESET_CNT is used as a timeout mechanism when resetting
 * a flash.  NAND flash is initialized prior to interrupts so standard timers
 * can't be used.  CONFIG_SYS_NAND_RESET_CNT should be set to a value
 * which is greater than (max NAND reset time / NAND status read time).
 * A conservative default of 200000 (500 us / 25 ns) is used as a default.
 */
#ifndef CONFIG_SYS_NAND_RESET_CNT
#define CONFIG_SYS_NAND_RESET_CNT 200000
#endif

static int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd,
			   int new_state);
static int nand_wait(struct mtd_info *mtd, struct nand_chip *this);

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd:	MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	this->select_chip(mtd, -1);	/* De-select the NAND device */
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static uint8_t nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return readb(chip->IO_ADDR_R);
}

#ifdef FLASH16BIT
/**
 * nand_read_byte16 - [DEFAULT] read one byte endianess aware from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith with
 * endianess conversion
 */
static uint8_t nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return (uint8_t) (readw(chip->IO_ADDR_R));
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith without
 * endianess conversion
 */
static u16 nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return readw(chip->IO_ADDR_R);
}
#endif

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chipnr:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;

	switch (chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:
		break;

/*	default:
		printk("BUG\n");
*/
	}
}
#if 0
/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; i < len; i++)
		if (buf[i] != readb(chip->IO_ADDR_R))
			return -EFAULT;
	return 0;
}
#endif

#ifdef FLASH16BIT
/**
 * nand_write_buf16 - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 16bit buswith
 */
static void nand_write_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		writew(p[i], chip->IO_ADDR_W);

}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 16bit buswith
 */
static void nand_read_buf16(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		p[i] = readw(chip->IO_ADDR_R);
}

/**
 * nand_verify_buf16 - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 16bit buswith
 */
static int nand_verify_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		if (p[i] != readw(chip->IO_ADDR_R))
			return -EFAULT;

	return 0;
}
#endif

/**
 * nand_check_wp - [GENERIC] check if the chip is write protected
 * @mtd:	MTD device structure
 * Check, if the device is write protected
 *
 * The function expects, that the device is already selected
 */
static int nand_check_wp(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	/* Check the WP bit */
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	return (chip->read_byte(mtd) & NAND_STATUS_WP) ? 0 : 1;
}

/*
 * Wait for the ready pin, after a command
 * The timeout is catched later.
 */
void nand_wait_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	/* u32 timeo = (CONFIG_SYS_HZ * 20) / 1000;

	reset_timer();
	*/

	/* wait until command is processed or timeout occures */
	while (1/*get_timer(0) < timeo*/) {
		if (chip->dev_ready)
			if (chip->dev_ready(mtd))
				break;
	}
}

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;
	uint32_t rst_sts_cnt = CONFIG_SYS_NAND_RESET_CNT;

	chip->cmd_ctrl(mtd, command, ctrl);

	/*
	 * Address cycle, when necessary
	 */
	ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;
	/* Serially input address */
	if (column != -1) {
#ifdef FLASH16BIT
		/* Adjust columns for 16 bit buswidth */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;
#endif
		chip->cmd_ctrl(mtd, column, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
		chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20))
			chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {
	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_CTRL_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd,
			       NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY) &&
			(rst_sts_cnt--));
		return;

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	nop(10);

	nand_wait_ready(mtd);
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices We dont have the separate regions as we have in the small page
 * devices.  We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static void nand_command_lp(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	uint32_t rst_sts_cnt = CONFIG_SYS_NAND_RESET_CNT;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command & 0xff,
		       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
#ifdef FLASH16BIT
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16)
				column >>= 1;
#endif
			chip->cmd_ctrl(mtd, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
			chip->cmd_ctrl(mtd, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8,
				       NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16,
					       NAND_NCE | NAND_ALE);
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
		return;

		/*
		 * read error status commands require only a short delay
		 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(chip->chip_delay);
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY) &&
			(rst_sts_cnt--));
		return;

	case NAND_CMD_RNDOUT:
		/* No ready / busy check necessary */
		chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		return;

	case NAND_CMD_READ0:
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	nop(10);

	nand_wait_ready(mtd);
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @chip:	the nand chip descriptor
 * @mtd:	MTD device structure
 * @new_state:	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int nand_get_device(struct nand_chip *this, struct mtd_info *mtd,
								int new_state)
{
	this->state = new_state;
	return 0;
}

/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @chip:	NAND chip structure
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to
 * general NAND and SmartMedia specs
 */
static int nand_wait(struct mtd_info *mtd, struct nand_chip *this)
{
	unsigned long	timeo;
	int state = this->state;

	if (state == FL_ERASING)
		timeo = (CONFIG_SYS_HZ * 400);/* / 1000; */
	else
		timeo = (CONFIG_SYS_HZ * 20);/* / 1000; */

	if ((state == FL_ERASING) && (this->options & NAND_IS_AND))
		this->cmdfunc(mtd, NAND_CMD_STATUS_MULTI, -1, -1);
	else
		this->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	/* reset_timer(); */

	while (timeo--) {
		/* if (get_timer(0) > timeo) { */
		if (timeo == 1) {
			printk("T.O.");
			return 0x01;
		}

		if (this->dev_ready) {
			if (this->dev_ready(mtd))
				break;
		} else {
			if (this->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
	}
#ifdef PPCHAMELON_NAND_TIMER_HACK
	/* reset_timer(); */
	while (get_timer(0) < 10);
#endif /*  PPCHAMELON_NAND_TIMER_HACK */

	return this->read_byte(mtd);
}

/**
 * nand_read_page_hwecc_oob_first - [REPLACABLE] hw ecc, read oob first
 * @mtd:        mtd info structure
 * @chip:       nand chip info structure
 * @buf:        buffer to store read data
 * @page:       page number to read
 *
 * Hardware ECC for large page chips, require OOB to be read first.
 * For this ECC mode, the write_page method is re-used from ECC_HW.
 * These methods read/write ECC from the OOB area, unlike the
 * ECC_HW_SYNDROME support with multiple ECC steps, follows the
 * "infix ECC" scheme and reads/writes ECC from the data area, by
 * overwriting the NAND manufacturer bad block markings.
 */
static int nand_read_page_hwecc_oob_first(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;

	/* Read the OOB area first */
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], NULL);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}
	return 0;
}

/**
 * nand_write_page_hwecc - [REPLACABLE] hardware ecc based page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
static void nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

/**
 * nand_write_page - [REPLACEABLE] write one page
 * @mtd:	MTD device structure
 * @chip:	NAND chip descriptor
 * @buf:	the data to write
 * @page:	page number to write
 * @cached:	cached programming
 * @raw:	use _raw version of write_page
 */
static int nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int page, int cached, int raw)
{
	int status;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);
	chip->ecc.write_page(mtd, chip, buf);

	/*
	 * Cached progamming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;

	if (!cached || !(chip->options & NAND_CACHEPRG)) {

		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

	return 0;
}

/**
 * single_erease_cmd - [GENERIC] NAND standard block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * Standard erase command for NAND chips
 */
static void single_erase_cmd(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	/* Send commands to erase a block */
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
}

/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 *
 * Erase one ore more blocks
 */
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand(mtd, instr, 0);
}

#define BBT_PAGE_MASK	0xffffff3f
/**
 * nand_erase_nand - [Internal] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 * @allowbbt:	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
		    int allowbbt)
{
	int page, status, pages_per_block, ret, chipnr;
	struct nand_chip *chip = mtd->priv;
	loff_t rewrite_bbt[CONFIG_SYS_NAND_MAX_CHIPS] = {0};
	unsigned int bbt_masked_page = 0xffffffff;
	loff_t len;

#ifdef VERBOSE
	printk("nand_erase: st = 0x%012llx, "
		 "l = %llu\n", (unsigned long long) instr->addr,
		 (unsigned long long) instr->len);
#endif

	/* Start address must align on block boundary */
	if (instr->addr & ((1 << chip->phys_erase_shift) - 1)) {
#ifdef VERBOSE
		printk("nand_er: Unalgn addr\n");
#endif
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (instr->len & ((1 << chip->phys_erase_shift) - 1)) {
#ifdef VERBOSE
		printk("nand_ere: Len not bl align\n");
#endif
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((instr->len + instr->addr) > mtd->size) {
#ifdef VERBOSE
		printk("nand_er: past end of dev\n");
#endif
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	/* Grab the lock and see if the device is available */
	nand_get_device(chip, mtd, FL_ERASING);

	/* Shift to get first page */
	page = (int)(instr->addr >> chip->page_shift);
	chipnr = (int)(instr->addr >> chip->chip_shift);

	/* Calculate pages in each block */
	pages_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

	/* Select the NAND device */
	chip->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd)) {
#ifdef VERBOSE
		printk("nand_er: Dev wr prot!\n");
#endif
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
		/*
		 * heck if we have a bad block, we do not erase bad blocks !
		 */
		/*
		 * Invalidate the page cache, if we erase the block which
		 * contains the current cached page
		 */
		if (page <= chip->pagebuf && chip->pagebuf <
		    (page + pages_per_block))
			chip->pagebuf = -1;

		chip->erase_cmd(mtd, page & chip->pagemask);

		status = chip->waitfunc(mtd, chip);

		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_ERASING,
					       status, page);

		/* See if block erase succeeded */
		if (status & NAND_STATUS_FAIL) {
#ifdef VERBOSE
			printk("nand_er: Fail pg 0x%08x\n", page);
#endif
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = ((loff_t)page << chip->page_shift);
			goto erase_exit;
		}

		/*
		 * If BBT requires refresh, set the BBT rewrite flag to the
		 * page being erased
		 */
		if (bbt_masked_page != 0xffffffff &&
		    (page & BBT_PAGE_MASK) == bbt_masked_page)
			rewrite_bbt[chipnr] =
				((loff_t)page << chip->page_shift);

		/* Increment page address and decrement length */
		len -= (1 << chip->phys_erase_shift);
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & chip->pagemask)) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);

		}
	}
	instr->state = MTD_ERASE_DONE;

 erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(mtd);

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/*
	 * If BBT requires refresh and erase was successful, rewrite any
	 * selected bad block tables
	 */
	if (bbt_masked_page == 0xffffffff || ret)
		return ret;

	for (chipnr = 0; chipnr < chip->numchips; chipnr++) {
		if (!rewrite_bbt[chipnr])
			continue;
	}

	/* Return more or less happy */
	return ret;
}

int read_page_update(struct mtd_info *mtd, int page, uint8_t *buf_ok,
		     uint8_t *buf_dummy)
{
	struct nand_chip *chip = mtd->priv;
	static int pageoffset;
	int i, page_ok, j, pages_per_block, page_tmp;
	uint8_t *buf_tmp;

	if (!pageoffset)
		pageoffset = U_BOOT_MAX_DIM / mtd->writesize;

	page_ok = 0;
	for (i = 0; i < U_BOOT_COPIES; i++) {
		uint8_t *buf;

		if (page_ok == 0)
			buf = buf_ok;
		else
			buf = buf_dummy;

		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page + i * pageoffset);

		mtd->ecc_stats.corrected = 0;
		mtd->ecc_stats.failed = 0;

		if (chip->ecc.read_page(mtd, chip, buf, page + i * pageoffset))
			continue;
		if ((*buf == 0xff) && (*(buf + 833) == 0xff) && (*(buf + 1548) == 0xff)) {
			/* think page is blank. If it isn't, it's not dangerous */
			if (i == (U_BOOT_COPIES - 1))
				j = 0;
			else
				j = i + 1;
			chip->ecc.read_page(mtd, chip, buf_dummy, page + j * pageoffset);
			nand_write_page(mtd, chip, buf_dummy, page + i * pageoffset, 0, 0);
			continue;
		}

		if (mtd->ecc_stats.corrected) {
			pages_per_block = mtd->erasesize / mtd->writesize;
			page_tmp = page & ~(pages_per_block - 1);
			buf_tmp = buf_dummy;
			for (j = 0; j < pages_per_block; j++) {
				chip->ecc.read_page(mtd, chip, buf_tmp,
						page_tmp + j + i * pageoffset);
				buf_tmp += mtd->writesize;
			}
			chip->erase_cmd(mtd, page_tmp + i * pageoffset);
			chip->waitfunc(mtd, chip);
			buf_tmp = buf_dummy;
			for (j = 0; j < pages_per_block; j++) {
				nand_write_page(mtd, chip, buf_tmp,
					page_tmp + j + i * pageoffset, 0, 0);
				chip->waitfunc(mtd, chip);
				buf_tmp += mtd->writesize;
			}
		} else if (mtd->ecc_stats.failed) {
			nop(1);
			continue;
		}

		page_ok = 1;
	}
	return 0;
}

/**
 * read_uboot_flash - Function for bubl-main in order to read u-boot from flash
 *
 * The function assume that u-boot is written 4 times at 384KBytes(0x60000)
 * All the pages are read from all copies and demaged copies are restored
 * @mtd:        MTD device structure
 * @start_page:	The page where to stard searching for u-boot
 * @start:	IN: pointer to a valid RAM area
 *		OUT: pointer to the start of RAM u-boot-copy
 */

long read_uboot_flash(struct mtd_info *mtd, int start_page,
				unsigned char *start)
{
	struct nand_chip *chip = mtd->priv;
	int page;
	uint32_t readlen;
	uint8_t *buf_ok, *buf_dummy, *uboot_entry_point;

	buf_dummy = start;
	nand_get_device(chip, mtd, FL_READING);

	chip->select_chip(mtd, 0);
	page = start_page & chip->pagemask;
	/* read u-boot header */
	read_page_update(mtd, page, buf_dummy, buf_dummy + mtd->erasesize);

	uboot_entry_point = buf_ok = (uint8_t *)((u32 *)buf_dummy)[0x10];

	readlen = 0;
	/* calculate the number of pages of u-boot */
	do {
		readlen += mtd->writesize;
	} while (readlen < ((u32 *)buf_dummy)[0x12]);
	/*buf[0x11] = _bss_start - _start ## buf[0x12] = _end - _start */

	if (readlen > U_BOOT_MAX_DIM)
		readlen = U_BOOT_MAX_DIM;

	/* read u-boot */
	memcpy(buf_ok, buf_dummy, mtd->writesize);
	buf_ok += mtd->writesize;
	do {
		page++;
		page &= chip->pagemask;
		readlen -= mtd->writesize;
		read_page_update(mtd, page, buf_ok, buf_dummy);
		buf_ok += mtd->writesize;
	} while (readlen);

	nand_wait_ready(mtd);
	nand_release_device(mtd);

	return (long) uboot_entry_point;
}

/*
 * Set default functions
 */
static void nand_set_defaults(struct nand_chip *chip, int busw)
{
	/* check for proper chip_delay setup, set 20us if not */
	if (!chip->chip_delay)
		chip->chip_delay = 20;
	/* check, if a user supplied command function given */
	if (chip->cmdfunc == NULL)
		chip->cmdfunc = nand_command;
	/* check, if a user supplied wait function given */
	if (chip->waitfunc == NULL)
		chip->waitfunc = nand_wait;

	if (!chip->select_chip)
		chip->select_chip = nand_select_chip;
	if (!chip->read_byte)
#ifdef FLASH16BIT
		chip->read_byte = busw ? nand_read_byte16 : nand_read_byte;
#else
		chip->read_byte = nand_read_byte;
#endif
}

/*
 * Get the flash and manufacturer id and lookup if the type is supported
 */
static struct nand_flash_dev *nand_get_flash_type(struct mtd_info *mtd,
						  struct nand_chip *chip,
						  int busw, int *maf_id)
{
	struct nand_flash_dev *type = NULL;
	int i, dev_id, maf_idx;
	int tmp_id, tmp_manf;

	/* Select the device */
	chip->select_chip(mtd, 1);

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */

	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	*maf_id = chip->read_byte(mtd);
	dev_id = chip->read_byte(mtd);

	/* Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */

	tmp_manf = chip->read_byte(mtd);
	tmp_id = chip->read_byte(mtd);

	if (tmp_manf != *maf_id || tmp_id != dev_id) {
#ifdef VERBOSE
		printk("init: sec ID rd not match "
		       "%02x,%02x against %02x,%02x\n",
		       *maf_id, dev_id, tmp_manf, tmp_id);
#endif
		return 0; /*-ENODEV;*/
	}

	/* Lookup the flash id */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
		if (dev_id == nand_flash_ids[i].id) {
			type =  &nand_flash_ids[i];
			break;
		}
	}

	if (!type) {
		/* supress warning if there is no nand */
		if (*maf_id != 0x00 && *maf_id != 0xff &&
		    dev_id  != 0x00 && dev_id  != 0xff)
#ifdef VERBOSE
			printk("init: unkn NAND dev: "
				"Manuf ID: 0x%02x, Ch ID: 0x%02x\n",
				*maf_id, dev_id);
#endif
		return 0; /*(-ENODEV);*/
	}

	if (!mtd->name)
		mtd->name = type->name;

	chip->chipsize = (uint64_t)type->chipsize << 20;

	int extid;
	/* The 3rd id byte holds MLC / multichip data */
	chip->cellinfo = chip->read_byte(mtd);
	/* The 4th id byte is the important one */
	extid = chip->read_byte(mtd);
	/* Calc pagesize */
	mtd->writesize = 1024 << (extid & 0x3);
	extid >>= 2;
	/* Calc oobsize */
	mtd->oobsize = (8 << (extid & 0x01)) * (mtd->writesize >> 9);
	extid >>= 2;
	/* Calc blocksize. Blocksize is multiples of 64KiB */
	mtd->erasesize = (64 * 1024) << (extid & 0x03);
	extid >>= 2;
#ifdef FLASH16BIT
	/* Get buswidth information */
	busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
#else
	busw = 0;
#endif

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->writesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift =
		ffs(mtd->erasesize) - 1;
	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32)) + 31;

	/* Set the bad block position */
	chip->badblockpos = mtd->writesize > 512 ?
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

	/* Get chip options, preserve non chip based options */
	chip->options &= ~NAND_CHIPOPTIONS_MSK;
	chip->options |= type->options & NAND_CHIPOPTIONS_MSK;

	/*
	 * Set chip as a default. Board drivers can override it, if necessary
	 */
	chip->options |= NAND_NO_AUTOINCR;

	/* Check if chip is a not a samsung device. Do not clear the
	 * options for chips which are not having an extended id.
	 */
	if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;
		chip->erase_cmd = single_erase_cmd;

	/* Do not replace user supplied command function ! */
	if (mtd->writesize > 512 && chip->cmdfunc == nand_command)
		chip->cmdfunc = nand_command_lp;

/*	printk("NAND dev: Manuf ID:"
		  " 0x%02x, Ch ID: 0x%02x (%s %s)\n", *maf_id, dev_id,
		  nand_manuf_ids[maf_idx].name, type->name);
*/
	return type;
}

/**
 * nand_scan_ident - [NAND Interface] Scan for the NAND device
 * @mtd:	     MTD device structure
 * @maxchips:	     Number of chips to scan for
 *
 * This is the first phase of the normal nand_scan() function. It
 * reads the flash ID and sets up MTD fields accordingly.
 *
 * The mtd->owner field must be set to the module of the caller.
 */
int nand_scan_ident(struct mtd_info *mtd, int maxchips)
{
	int i, busw, nand_maf_id;
	struct nand_chip *chip = mtd->priv;
	struct nand_flash_dev *type;

#ifdef FLASH16BIT
	/* Get buswidth to select the correct functions */
	busw = chip->options & NAND_BUSWIDTH_16;
#else
	busw = 0;
#endif
	/* Set the default functions */
	nand_set_defaults(chip, busw);

	/* Read the flash type */
	type = nand_get_flash_type(mtd, chip, busw, &nand_maf_id);

	if (!type) {
#ifndef VERBOSE
		printk("No NAND dev found!\n");
#endif
		chip->select_chip(mtd, -1);
		return -ENODEV;
	}

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		chip->select_chip(mtd, i);
		/* See comment in nand_get_flash_type for reset */
		chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		/* Send the command for reading device ID */
		chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */
		if (nand_maf_id != chip->read_byte(mtd) ||
		    type->id != chip->read_byte(mtd))
			break;
	}
#ifdef DEBUG
	if (i > 1)
		printk("%d NAND chips det\n", i);
#endif

	/* Store the number of chips and calc total size for mtd */
	chip->numchips = i;
	mtd->size = i * chip->chipsize;

	return 0;
}

/**
 * nand_scan_tail - [NAND Interface] Scan for the NAND device
 * @mtd:	    MTD device structure
 *
 * This is the second phase of the normal nand_scan() function. It
 * fills out all the uninitialized function pointers with the defaults
 * and scans for a bad block table if appropriate.
 */
int nand_scan_tail(struct mtd_info *mtd)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	if (!(chip->options & NAND_OWN_BUFFERS))
		/* point statically to a RAM region in order to limitate bss */
		chip->buffers = (struct nand_buffers *) 0x82000000;

	/* Set the internal oob buffer location, just after the page data */
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;

	/*
	 * If no default placement scheme is given, select an appropriate one
	 */
		chip->write_page = nand_write_page;

	/*
	 * check ECC mode, default to software if 3byte/512byte hardware ECC is
	 * selected and we have 256 byte pagesize fallback to software ECC
	 */

	chip->ecc.mode = NAND_ECC_HW_OOB_FIRST;
		/* Use standard hwecc read page function ? */
	chip->ecc.read_page = nand_read_page_hwecc_oob_first;
	chip->ecc.write_page = nand_write_page_hwecc;

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area
	 */
	chip->ecc.layout->oobavail = 0;
	for (i = 0; chip->ecc.layout->oobfree[i].length
			&& i < ARRAY_SIZE(chip->ecc.layout->oobfree); i++)
		chip->ecc.layout->oobavail +=
			chip->ecc.layout->oobfree[i].length;
	mtd->oobavail = chip->ecc.layout->oobavail;

	/*
	 * Set the number of read / write steps for one page depending on ECC
	 * mode
	 */
	chip->ecc.steps = mtd->writesize / chip->ecc.size;
	if (chip->ecc.steps * chip->ecc.size != mtd->writesize) {
#ifdef VERBOSE
		printk("Inv ecc par\n");
		BUG();
#endif
	}
	chip->ecc.total = chip->ecc.steps * chip->ecc.bytes;

	/* Initialize state */
	chip->state = FL_READY;

	/* De-select the device */
	chip->select_chip(mtd, -1);

	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->erase = nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->lock = NULL;
	mtd->unlock = NULL;

	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = chip->ecc.layout;

	return 0;
}

/**
 * nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd:	MTD device structure
 * @maxchips:	Number of chips to scan for
 *
 * This fills out all the uninitialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 * The mtd->owner field must be set to the module of the caller
 *
 */
int nand_scan(struct mtd_info *mtd, int maxchips)
{
	int ret;

	ret = nand_scan_ident(mtd, maxchips);
	if (!ret)
		ret = nand_scan_tail(mtd);
	return ret;
}

