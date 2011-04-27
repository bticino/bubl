/*
 * There is some yet-unknown-to-me hardware to set up
 */

extern int misc_setup0(void);
extern int misc_setup1(void);
extern void pinmux_setup(int bootmode);

/* See sprufb3.pdf, Chapter 8 */
typedef struct _DEVICE_AINTC_REGS_
{
  VUint32 FIQ0;         //0x00
  VUint32 FIQ1;         //0x04
  VUint32 IRQ0;         //0x08
  VUint32 IRQ1;         //0x0c
  VUint32 FIQENTRY;     //0x10
  VUint32 IRQENTRY;     //0x14
  VUint32 EINT0;        //0x18
  VUint32 EINT1;        //0x1c
  VUint32 INTCTL;       //0x20
  VUint32 EABASE;       //0x24
  VUint8 RSVD0[8];      //0x28
  VUint32 INTPRI0;      //0x30
  VUint32 INTPRI1;      //0x34
  VUint32 INTPRI2;      //0x38
  VUint32 INTPRI3;      //0x3c
  VUint32 INTPRI4;      //0x40
  VUint32 INTPRI5;      //0x44
  VUint32 INTPRI6;      //0x48
  VUint32 INTPRI7;      //0x4c
}
DEVICE_AIntcRegs;

#define AINTC ((DEVICE_AIntcRegs*) 0x01C48000)

typedef struct _DEVICE_GPIO_REGS_
{
  VUint32 PID;         //0x00
  VUint32 RSVD1;           //0x4
  VUint32 BINTEN;        //0x8
  VUint32 RSVD2;       //0xC
  VUint32 DIR01;          //0x10
  VUint32 OUTDATA01;            //0x14
  VUint32 SETDATA01;         //0x18
  VUint32 CLRDATA01;       //0x1C
  VUint32 INDTATA01;       //0x20
  VUint32 SETRIS01;        //0x24
  VUint32 CLRRIS01;              //0x28
  VUint32 SETFAL01;         //0x2C
  VUint32 CLRFAL01;       //0x30
  VUint32 INTSTAT01;      //0x34
  VUint32 DIR02;          //0x10
  VUint32 OUTDATA02;            //0x14
  VUint32 SETDATA02;         //0x18
  VUint32 CLRDATA02;       //0x1C
  VUint32 INDTATA02;       //0x20
  VUint32 SETRIS02;        //0x24
  VUint32 CLRRIS02;              //0x28
  VUint32 SETFAL02;         //0x2C
  VUint32 CLRFAL02;       //0x30
  VUint32 INTSTAT02;      //0x34
}
DEVICE_GPIORegs;

#define GPIO ((DEVICE_GPIORegs*) 0x01C67000)


// AEMIF Register structure - See sprued1b.pdf for more details.
typedef struct _DEVICE_EMIF_REGS_
{
  VUint32 ERCSR;             // 0x00
  VUint32 AWCCR;             // 0x04
  VUint8  RSVD0[8];          // 0x08
  VUint32 A1CR;              // 0x10
  VUint32 A2CR;              // 0x14
  VUint8  RSVD1[40];         // 0x18
  VUint32 EIRR;              // 0x40
  VUint32 EIMR;              // 0x44
  VUint32 EIMSR;             // 0x48
  VUint32 EIMCR;             // 0x4C
  VUint8  RSVD2[12];         // 0x50
  VUint32 ONENANDCTL;        // 0x5C
  VUint32 NANDFCR;           // 0x60
  VUint32 NANDFSR;           // 0x64
  VUint8  RSVD3[8];          // 0x68
  VUint32 NANDF1ECC;         // 0x70
  VUint32 NANDF2ECC;         // 0x74
  VUint8  RSVD4[68];         // 0x78
  VUint32 NAND4BITECCLOAD;   // 0xBC
  VUint32 NAND4BITECC1;      // 0xC0
  VUint32 NAND4BITECC2;      // 0xC4
  VUint32 NAND4BITECC3;      // 0xC8
  VUint32 NAND4BITECC4;      // 0xCC
  VUint32 NANDERRADD1;       // 0xD0
  VUint32 NANDERRADD2;       // 0xD4
  VUint32 NANDERRVAL1;       // 0xD8
  VUint32 NANDERRVAL2;       // 0xDC
}
DEVICE_EmifRegs;

//The memory map is different for DM35x and DM36x w.r.t AEMIF
#define AEMIF ((DEVICE_EmifRegs*) 0x01D10000)

// Power/Sleep Ctrl Register structure - See sprufb3.pdf, Chapter 7
typedef struct _DEVICE_PSC_REGS_
{
  VUint32 PID;        // 0x000
  VUint8 RSVD0[16];   // 0x004
  VUint8 RSVD1[4];    // 0x014
  VUint32 INTEVAL;    // 0x018
  VUint8 RSVD2[36];   // 0x01C
  VUint32 MERRPR0;    // 0x040
  VUint32 MERRPR1;    // 0x044
  VUint8 RSVD3[8];    // 0x048
  VUint32 MERRCR0;    // 0x050
  VUint32 MERRCR1;    // 0x054
  VUint8 RSVD4[8];    // 0x058
  VUint32 PERRPR;     // 0x060
  VUint8 RSVD5[4];    // 0x064
  VUint32 PERRCR;     // 0x068
  VUint8 RSVD6[4];    // 0x06C
  VUint32 EPCPR;      // 0x070
  VUint8 RSVD7[4];    // 0x074
  VUint32 EPCCR;      // 0x078
  VUint8 RSVD8[144];  // 0x07C
  VUint8 RSVD9[20];   // 0x10C
  VUint32 PTCMD;      // 0x120
  VUint8 RSVD10[4];   // 0x124
  VUint32 PTSTAT;     // 0x128
  VUint8 RSVD11[212]; // 0x12C
  VUint32 PDSTAT0;    // 0x200
  VUint32 PDSTAT1;    // 0x204
  VUint8 RSVD12[248]; // 0x208
  VUint32 PDCTL0;     // 0x300
  VUint32 PDCTL1;     // 0x304
  VUint8 RSVD13[536]; // 0x308
  VUint32 MCKOUT0;    // 0x520
  VUint32 MCKOUT1;    // 0x524
  VUint8 RSVD14[728]; // 0x528
  VUint32 MDSTAT[52]; // 0x800
  VUint8 RSVD15[304]; // 0x8D0
  VUint32 MDCTL[52];  // 0xA00
}
DEVICE_PSCRegs;

#define PSC ((DEVICE_PSCRegs*) 0x01C41000)


// PSC constants
#define LPSC_TPCC		(0)
#define LPSC_TPTC0		(1)
#define LPSC_TPTC1		(2)
#define LPSC_TPTC2		(3)
#define LPSC_TPTC3		(4)
#define LPSC_TIMER3		(5)
#define LPSC_SPI1		(6)
#define LPSC_MMC_SD1		(7)
#define LPSC_ASP1		(8)
#define LPSC_USB		(9)
#define LPSC_PWM3		(10)
#define LPSC_SPI2		(11)
#define LPSC_RTO		(12)
#define LPSC_DDR2		(13)
#define LPSC_AEMIF		(14)
#define LPSC_MMC_SD0		(15)
#define LPSC_MEMSTK		(16)
#define TIMER4			(17)
#define LPSC_I2C		(18)
#define LPSC_UART0		(19)
#define LPSC_UART1		(20)
#define LPSC_UHPI		(21)
#define LPSC_SPIO		(22)
#define LPSC_PWM0		(23)
#define LPSC_PWM1		(24)
#define LPSC_PWM2		(25)
#define LPSC_GPIO		(26)
#define LPSC_TIMER0		(27)
#define LPSC_TIMER1		(28)
#define LPSC_TIMER2		(29)
#define LPSC_SYSMOD		(30)
#define LPSC_ARM		(31)
#define LPSC_SPI3		(38)
#define LPSC_SPI4		(39)
#define LPSC_CPGMAC		(40)
#define LPSC_RTC		(41)
#define LPSC_KEYSCAN		(42)
#define LPSC_ADC		(43)
#define LPSC_VOICECODEC		(44)
#define LPSC_IMCOP		(50)
#define LPSC_KALEIDO		(51)

#define PSC_ENABLE		(0x3)
#define PSC_DISABLE		(0x2)
#define PSC_SYNCRESET		(0x1)
#define PSC_SWRSTDISABLE	(0x0)
