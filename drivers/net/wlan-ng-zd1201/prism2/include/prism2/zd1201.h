#ifndef _ZYDAS_H
#define _ZYDAS_H

/*
 * The parameters to enable and disable the AUX port
 * are different for the ZD1201.  Redefine them here.
 */
#undef		HFA384x_AUXPW0
#undef		HFA384x_AUXPW1
#undef		HFA384x_AUXPW2

#define		HFA384x_AUXPW0			((UINT16)0x55aa)
#define		HFA384x_AUXPW1			((UINT16)0x44bb)
#define		HFA384x_AUXPW2			((UINT16)0x33cc)

/*
 * There seems to be less memory for the TX stack
 * on the ZD1201 than there is on a standard PRISM.
 * Redefine so that we have a stack of 5 buffers.
 */
#undef HFA384x_DRVR_FIDSTACKLEN_MAX

#define		HFA384x_DRVR_FIDSTACKLEN_MAX	(6)

#define     ZD1201_HALT_CPU      0x0422
#define     ZD1201_RESUME_CPU    0x0022

/*
 * This time is required to wait for the firmware to
 * boot up after resuming the cpu.  The value is in centiseconds.
 */
#define     ZD1201_WAKEUP_TIME    5

#define ZD1201_FIRMWARE_OFFSET          (0x00000)
#define ZD1201_WATERMARK_PLAIN_OFFSET   (0x1FD00)
#define ZD1201_WATERMARK_CIPHER_OFFSET  (0x1FD80)

#define ZD1201_FIRMWARE_BUF_SZ		(64 * 1024)
#define ZD1201_FIRMWARE_MTD_DEV		"/dev/mtd1"

#define ZD1201_SMC_WAITSTATES_PCMCIA_FAST \
		((0x01 << 28) |   /* width == 16bit */         \
		(  20 << 11) |   /* write waitstates */        \
		(  20 <<  5) |   /* read  waitstates */        \
		(   4 <<  0))    /* turnaround waitstates */

#define ZD1201_SMC_WAITSTATES_PCMCIA_SLOW \
		((0x01 << 28) |   /* width == 16bit */         \
		(  30 << 11) |   /* write waitstates */        \
		(  30 <<  5) |   /* read  waitstates */        \
		(   4 <<  0))    /* turnaround waitstates */

static const unsigned short zd1201_watermark_plaintext[] =
{
	0x795a,0x4144,0x2053,0x7250,0x706f,0x7265,0x7974,0x0000, /* 0x1fd00 */
};
                                                                                
static const unsigned short zd1201_watermark_ciphertext[] =
{
	0x554c,0x4b48,0x5943,0x4a31	/* 0x1fd80 */
};

#endif /* _ZYDAS_H */
