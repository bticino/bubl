#ifndef __BUBL_HW_H__
#define __BUBL_HW_H__
#include <bubl/types.h>
/*
 * Was mostly form device.h by Sandeep Paulraj, in dm3xx_sd_boot-6.1 package.
 */


/* crap crap crap */
typedef volatile u32	VUint32;
typedef volatile u8	VUint8;


// System Control Module register structure for DM365
typedef struct _DEVICE_SYS_MODULE_REGS_
{
  VUint32 PINMUX[5];         //0x00
  VUint32 BOOTCFG;           //0x14
  VUint32 ARM_INTMUX;        //0x18
  VUint32 EDMA_EVTMUX;       //0x1C
  VUint32 DDR_SLEW;          //0x20
  VUint32 CLKOUT;            //0x24
  VUint32 DEVICE_ID;         //0x28
  VUint32 VDAC_CONFIG;       //0x2C
  VUint32 TIMER64_CTL;       //0x30
  VUint32 USBPHY_CTL;        //0x34
  VUint32 MISC;              //0x38
  VUint32 MSTPRI[2];         //0x3C
  VUint32 VPSS_CLKCTL;       //0x44
  VUint32 PERI_CLKCTRL;      //0x48
  VUint32 DEEPSLEEP;         //0x4C
  VUint32 DFT_ENABLE;        //0x50
  VUint32 DEBOUNCE[8];           //0x54
  VUint32 VTPIOCR;                       //0x74
  VUint32 PUPDCTL0;          //0x78
  VUint32 PUPDCTL1;          //0x7C
  VUint32 HDIMCOPBT;             //0x80
  VUint32 PLL0_CONFIG;       //0x84
  VUint32 PLL1_CONFIG;       //0x88
}
DEVICE_SysModuleRegs;

#define SYSTEM ((DEVICE_SysModuleRegs*) 0x01C40000)

// DDR2 Memory Ctrl Register structure - See sprueh7d.pdf for more details.
typedef struct _DEVICE_DDR2_REGS_
{
  VUint8 RSVD0[4];        //0x00
  VUint32 SDRSTAT;        //0x04
  VUint32 SDBCR;          //0x08
  VUint32 SDRCR;          //0x0C
  VUint32 SDTIMR;         //0x10
  VUint32 SDTIMR2;        //0x14
  VUint8 RSVD1[4];        //0x18
  VUint32 SDBCR2;         //0x1C
  VUint32 PBBPR;          //0x20
  VUint8 RSVD2[156];      //0x24
  VUint32 IRR;            //0xC0
  VUint32 IMR;            //0xC4
  VUint32 IMSR;           //0xC8
  VUint32 IMCR;           //0xCC
  VUint8 RSVD3[20];       //0xD0
  VUint32 DDRPHYCR;       //0xE4
  VUint32 DDRPHYCR2;       //0xE8
  VUint8 RSVD4[4];        //0xEC
}
DEVICE_DDR2Regs;

#define DDR                         ((DEVICE_DDR2Regs*) 0x20000000)

#define RAMADDR			0x80000000

extern int ddr_setup(void);


static inline void trivial_loop(int i)
{
	volatile int j = i;
	while (j--)
		;
}

#endif /* __BUBL_HW_H__ */
