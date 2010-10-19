#include <bubl/types.h>
#include <bubl/tools.h>
#include <mmcsd.h>

MMCSD_ConfigData cfg = {
	.writeEndian = MMCSD_LITTLE_ENDIAN,
	.readEndian = MMCSD_LITTLE_ENDIAN,
	.dat3Detect = MMCSD_DAT3_DISABLE,
	.spiModeEnable = 0,
	.busWidth = MMCSD_DATA_BUS_4,
	.timeoutResponse = 0xFF,
	.timeoutRead = 0xFFFF,
	.ishc = 0,
};

/* This is partly from sdcard_flash.c, by Constantine Shulyupin, GNU GPL */
int sdcard_init(void)
{
	unsigned int relCardAddr = 0;
	MMCSD_cardStatusReg cardStatus;

	if (MMCSD_initCard(&relCardAddr, &cardStatus, &cfg,
			   MMCSD_FIFOLEVEL_32BYTES)) {
		printk("SD Card Initialization failed\n");
		return -1;
	}
	printk("%s: success (%cC)\n", __func__, cfg.ishc ? 'H' : 'S');
	return 0;
}

int sdcard_read_block(int blknr, unsigned long addr)
{
    /* first arg limited to 4GB, last argument "endian" is not used */
    return MMCSD_singleBlkRead(blknr << 9, (void *)addr, 512, 0);
}
