/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * cirrus.c
 * 
 * tab:8
 * 
 * Original copyrights:
 *      "SVGAlib, Copyright 1993 Harm Hanemaayer"
 *      "VGAlib version 1.2 - (c) 1993 Tommy Frandsen"
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      cirrus.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Adaptaed from cirrus.c of SVGAlib 1.4.3. Final revision.
 *        Fix all warnings.
 * 
 */


#include "../macros.h"
#include "vga.h"
#include "libvga.h"
#include "timing.h"
#include "vgaregs.h"
#include "interface.h"
#include "accel.h"
#include "cirrus.h"
#include "macros.h"

#define SUPPORT_5434_PALETTE_CLOCK_DOUBLING

/* remember to check WARNNING */

#define CIRRUSREG_GR(i) (VGA_TOTAL_REGS + i - VGA_GRAPHICS_COUNT)
#define CIRRUSREG_SR(i) (VGA_TOTAL_REGS + 5 + i - VGA_SEQUENCER_COUNT)
#define CIRRUSREG_CR(i) (VGA_TOTAL_REGS + 5 + 27 + i - VGA_CRTC_COUNT)
#define CIRRUSREG_DAC (VGA_TOTAL_REGS + 5 + 27 + 15)
#define CIRRUS_TOTAL_REGS (VGA_TOTAL_REGS + 5 + 27 + 15 + 1)

/* Indices into mode register array. */
#define CIRRUS_GRAPHICSOFFSET1	CIRRUSREG_GR(0x09)
#define CIRRUS_GRAPHICSOFFSET2	CIRRUSREG_GR(0x0A)
#define CIRRUS_GRB		CIRRUSREG_GR(0x0B)
#define CIRRUS_SR7		CIRRUSREG_SR(0x07)
#define CIRRUS_VCLK3NUMERATOR	CIRRUSREG_SR(0x0E)
#define CIRRUS_DRAMCONTROL	CIRRUSREG_SR(0x0F)
#define CIRRUS_PERFTUNING	CIRRUSREG_SR(0x16)
#define CIRRUS_SR17		CIRRUSREG_SR(0x17)
#define CIRRUS_VCLK3DENOMINATOR CIRRUSREG_SR(0x1E)
#define CIRRUS_MCLKREGISTER	CIRRUSREG_SR(0x1F)
#define CIRRUS_CR19		CIRRUSREG_CR(0x19)
#define CIRRUS_CR1A		CIRRUSREG_CR(0x1A)
#define CIRRUS_CR1B		CIRRUSREG_CR(0x1B)
#define CIRRUS_CR1D 		CIRRUSREG_CR(0x1D)
#define CIRRUS_HIDDENDAC	CIRRUSREG_DAC


info_t* cur_vga_info;

/* Indentify chipset, initialize and return non-zero if detected */

static int cirrus_memory;
static int cirrus_chiptype;
static int cirrus_chiprev;
static int cirrus_pci_linear = 0;
static unsigned char actualMCLK, programmedMCLK;
static int DRAMbandwidth, DRAMbandwidthLimit;

static CardSpecs __cardspecs;
static CardSpecs* cardspecs = &__cardspecs;

#define NU_FIXED_CLOCKS 21

/* 12.588 clock (0x33, 0x3B) replaced with 12.599 (0x2C, 0x33). */

static int cirrus_fixed_clocks[NU_FIXED_CLOCKS] =
{
    12599, 18000, 19600,
    25227, 28325, 31500, 36025, 37747, 39992, 41164,
    45076, 49867, 64983, 72163, 75000, 80013, 85226, 89998,
    95019, 100226, 108035
};

static unsigned char fixed_clock_numerator[NU_FIXED_CLOCKS] =
{
    0x2C, 0x27, 0x28,
    0x4A, 0x5B, 0x42, 0x4E, 0x3A, 0x51, 0x45,
    0x55, 0x65, 0x76, 0x7E, 0x6E, 0x5F, 0x7D, 0x58,
    0x49, 0x46, 0x53
};

static unsigned char fixed_clock_denominator[NU_FIXED_CLOCKS] =
{
    0x33, 0x3e, 0x3a,
    0x2B, 0x2F, 0x1F, 0x3E, 0x17, 0x3A, 0x30,
    0x36, 0x3A, 0x34, 0x32, 0x2A, 0x22, 0x2A, 0x1C,
    0x16, 0x14, 0x16
};

/* Efficient chip type checks. */

#define CHECKCHIP1(c1) ((1 << cirrus_chiptype) & (1 << c1))
#define CHECKCHIP2(c1, c2) ((1 << cirrus_chiptype) & ((1 << c1) | (1 << c2)))
#define CHECKCHIP3(c1, c2, c3) ((1 << cirrus_chiptype) & ((1 << c1) \
	| (1 << c2) | (1 << c3)))
#define CHECKCHIP4(c1, c2, c3, c4) ((1 << cirrus_chiptype) & ((1 << c1) \
	| (1 << c2) | (1 << c3) | (1 << c4)))
#define CHECKCHIP5(c1, c2, c3, c4, c5) ((1 << cirrus_chiptype) & \
	((1 << c1) | (1 << c2) | (1 << c3) | (1 << c4) | (1 << c5)))
#define CHECKCHIP6(c1, c2, c3, c4, c5, c6) ((1 << cirrus_chiptype) & \
	((1 << c1) | (1 << c2) | (1 << c3) | (1 << c4) | (1 << c5) \
	| (1 << c6)))
#define CHECKCHIP7(c1, c2, c3, c4, c5, c6, c7) ((1 << cirrus_chiptype) & \
	((1 << c1) | (1 << c2) | (1 << c3) | (1 << c4) | (1 << c5) \
	| (1 << c6) | (1 << c7))) 
#define CHECKCHIP8(c1, c2, c3, c4, c5, c6, c7, c8) ((1 << cirrus_chiptype) & \
	((1 << c1) | (1 << c2) | (1 << c3) | (1 << c4) | (1 << c5) \
	| (1 << c6) | (1 << c7) | (1 << c8)))
#define CHECKCHIP9(c1, c2, c3, c4, c5, c6, c7, c8, c9) ((1 << cirrus_chiptype) & \
	((1 << c1) | (1 << c2) | (1 << c3) | (1 << c4) | (1 << c5) \
	| (1 << c6) | (1 << c7) | (1 << c8) | (1 << c9)))
#define CHECKCHIPGREATEREQUAL(c) (cirrus_chiptype >= c)
#define CHECKCHIPNOTEQUAL(c) (cirrus_chiptype != c)

#define CHIP_HAS_CR1D() \
	CHECKCHIP4(CLGD5429, CLGD5430, CLGD5434, CLGD5436)
#define CHIP_HAS_GRC_AND_GRD() \
	CHECKCHIP4(CLGD5424, CLGD5426, CLGD5428, CLGD5429)
#define CHIP_HAS_GRE() \
	CHECKCHIP5(CLGD5428, CLGD5429, CLGD5430, CLGD5434, CLGD5436)
#define CHIP_HAS_GR10_AND_GR11() \
	CHECKCHIP7(CLGD5424, CLGD5426, CLGD5428, CLGD5429, CLGD5430, CLGD5434, CLGD5436)
#define CHIP_HAS_BLTTRANSPARENTCOLOR() \
	CHECKCHIP2(CLGD5426, CLGD5428)
#define CHIP_HAS_PERFTUNINGREGISTER() \
	CHECKCHIP7(CLGD5424, CLGD5426, CLGD5428, CLGD5429, CLGD5430, CLGD5434, CLGD5436)
#define CHIP_HAS_MCLK_REGISTER() \
	CHECKCHIP9(CLGD5420B, CLGD5422C, CLGD5424, CLGD5426, CLGD5428, \
		CLGD5429, CLGD5430, CLGD5434, CLGD5436)
#define CHIP_HAS_32BIT_DRAM_BUS() \
	CHECKCHIPGREATEREQUAL(CLGD5420B)
#define CHIP_HAS_64BIT_DRAM_BUS() \
	CHECKCHIP3(CLGD5434, CLGD5436, CLGD7548)
#define CHIP_HAS_HIDDENDAC() \
	CHECKCHIPGREATEREQUAL(CLGD5420B)
#define CHIP_HAS_ACCELERATION() \
	CHECKCHIPNOTEQUAL(CLGD5420B)
#define CHIP_HAS_SR17() \
	CHECKCHIPGREATEREQUAL(CLGD5422)
#define CHIP_USE_SR17() \
	CHECKCHIPGREATEREQUAL(CLGD5429)

enum {
    CLGD5420 = 0, CLGD7548, CLGD5420B, CLGD5422, CLGD5422C, CLGD5424, CLGD5426,
    CLGD5428, CLGD5429, CLGD5430, CLGD5434, CLGD5436
};

static int cirrus_init(int, int, int);
static void cirrus_unlock(void);

void __svgalib_cirrusaccel_init(void* accelspecs, int bpp, int width_in_pixels);


int __svgalib_cirrus_inlinearmode(void)
{
    outb(SEQ_I, 0x07);
    return (inb(SEQ_D) & 0xf0) != 0;
}

int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    /* update misc output register */
    __svgalib_outmisc(regs[MIS]);

    /* synchronous reset on */
    __svgalib_outseq(0x00,0x01);

    /* write sequencer registers */
    __svgalib_outseq(0x01,regs[SEQ + 1] | 0x20);
    port_out(1, SEQ_I);
    port_out(regs[SEQ + 1] | 0x20, SEQ_D);
    for (i = 2; i < SEQ_C; i++) {
       __svgalib_outseq(i,regs[SEQ + i]);
    }

    /* synchronous reset off */
    __svgalib_outseq(0x00,0x03);

    /* Unlock chipset-specific registers */
    __svgalib_outcrtc(0x11, __svgalib_incrtc(0x11) &0x7f);

    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[CRT + i]);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
	port_out(i, GRA_I);
	port_out(regs[GRA + i], GRA_D);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
	port_in(__svgalib_IS1_R);		/* reset flip-flop */
	__svgalib_delay();
	port_out(i, ATT_IW);
	__svgalib_delay();
	port_out(regs[ATT + i], ATT_IW);
	__svgalib_delay();
    }

    return 0;
}

/* Fill in chipset specific mode information */
/* not used */
// static void cirrus_getmodeinfo(int mode, vga_modeinfo * modeinfo)
// {
//     if (modeinfo->bytesperpixel > 0)
// 	modeinfo->maxpixels = cirrus_memory * 1024 / modeinfo->bytesperpixel;
//     else
// 	/* 16-color SVGA mode */
// 	/* Value taken from the air. */
// 	modeinfo->maxpixels = cirrus_memory * 2048;
//     modeinfo->maxlogicalwidth = 4088;
// #if 0
//     if (mode != G320x200x256) {
// 	/* No need to check for 320x200x256, we now have a special */
// 	/* SVGA-derived 320x200x256 mode that fully supports page */
// 	/* flipping etc. */
// #endif
// 	modeinfo->startaddressrange = cirrus_memory * 1024 - 1;
// 	modeinfo->haveblit = 0;
// #if 0
//     } else {
// 	modeinfo->startaddressrange = 0xffff;
// 	modeinfo->maxpixels = 65536;
// 	modeinfo->haveblit = 0;
//     }
// #endif
// #if 0				/* Who cares. */
//     if (cirrus_interlaced(mode))
// 	modeinfo->flags |= IS_INTERLACED;
// #endif
//     modeinfo->flags &= ~HAVE_RWPAGE;

//     if (modeinfo->bytesperpixel >= 1) {
// 	modeinfo->flags |= CAPABLE_LINEAR;
// 	if (__svgalib_cirrus_inlinearmode())
// 	    modeinfo->flags |= IS_LINEAR;
//     }
// }


/* Read and save chipset-specific registers */

static int cirrus_saveregs(unsigned char regs[])
{
/*      int i; */

/*      #ifdef DEBUG
   printf("Saving Cirrus extended registers.\n");
   #endif
 */
    cirrus_unlock();		/* May be locked again by other programs (e.g. X) */

#if 0
    /* Save extended CRTC registers. */
    for (i = 0; i < 15; i++) {	/* was 4 */
	port_out(0x18 + i, __svgalib_CRT_I);
	regs[CIRRUSREG_CR(i)] = port_in(__svgalib_CRT_D);
    }

    /* Save extended graphics registers */
    for (i = 0; i < 5; i++) {	/* was 3 */
	port_out(0x09 + i, GRA_I);
	regs[CIRRUSREG_GR(i)] = port_in(GRA_D);
    }

    /* Save extended sequencer registers. */
    for (i = 0; i < 27; i++) {
	port_out(0x05 + i, SEQ_I);
	regs[CIRRUSREG_SR(i)] = port_in(SEQ_D);
    }
#endif

    /* Save extended CRTC registers. */
    regs[CIRRUSREG_CR(0x19)] = __svgalib_inCR(0x19);
    regs[CIRRUSREG_CR(0x1A)] = __svgalib_inCR(0x1A);
    regs[CIRRUSREG_CR(0x1B)] = __svgalib_inCR(0x1B);
    if (CHIP_HAS_CR1D())
	regs[CIRRUSREG_CR(0x1D)] = __svgalib_inCR(0x1D);

    /* Save extended graphics registers. */
    regs[CIRRUSREG_GR(0x09)] = __svgalib_inGR(0x09);
    regs[CIRRUSREG_GR(0x0A)] = __svgalib_inGR(0x0A);
    regs[CIRRUSREG_GR(0x0B)] = __svgalib_inGR(0x0B);

    /* Save extended sequencer registers. */
    regs[CIRRUS_SR7] = __svgalib_inSR(0x07);
    regs[CIRRUS_VCLK3NUMERATOR] = __svgalib_inSR(0x0E);
    regs[CIRRUS_DRAMCONTROL] = __svgalib_inSR(0x0F);
    if (CHIP_HAS_PERFTUNINGREGISTER())
	regs[CIRRUS_PERFTUNING] = __svgalib_inSR(0x16);
    if (CHIP_HAS_SR17())
	regs[CIRRUS_SR17] = __svgalib_inSR(0x17);
    regs[CIRRUS_VCLK3DENOMINATOR] = __svgalib_inSR(0x1E);
    if (CHIP_HAS_MCLK_REGISTER())
	regs[CIRRUS_MCLKREGISTER] = __svgalib_inSR(0x1F);

    /* Save Hicolor DAC register. */
    if (CHIP_HAS_HIDDENDAC()) {
	outb(0x3c6, 0);
	outb(0x3c6, 0xff);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	regs[CIRRUSREG_DAC] = inb(0x3c6);
    }
    return CIRRUS_TOTAL_REGS - VGA_TOTAL_REGS;
}

static void writehicolordac(unsigned char c)
{
    outb(0x3c6, 0);
    outb(0x3c6, 0xff);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    outb(0x3c6, c);
    inb(0x3c8);
}


/* Set chipset-specific registers */

static void cirrus_setregs(const unsigned char regs[], int mode)
{
/*      #ifdef DEBUG
   printf("Setting Cirrus extended registers.\n");
   #endif
 */
    cirrus_unlock();		/* May be locked again by other programs (eg. X) */

    /* Write extended CRTC registers. */
    __svgalib_outCR(0x19, regs[CIRRUSREG_CR(0x19)]);
    __svgalib_outCR(0x1A, regs[CIRRUSREG_CR(0x1A)]);
    __svgalib_outCR(0x1B, regs[CIRRUSREG_CR(0x1B)]);
    if (CHIP_HAS_CR1D())
	__svgalib_outCR(0x1D, regs[CIRRUSREG_CR(0x1D)]);

    /* Write extended graphics registers. */
    __svgalib_outGR(0x09, regs[CIRRUSREG_GR(0x09)]);
    __svgalib_outGR(0x0A, regs[CIRRUSREG_GR(0x0A)]);
    __svgalib_outGR(0x0B, regs[CIRRUSREG_GR(0x0B)]);

#ifdef NOT_SET_ALL /* used to be SET_ALL WARNING */
    if (CHIP_HAS_GRC_AND_GRD()) {
	__svgalib_outGR(0x0C, regs[CIRRUSREG_GR(0x0C)]);
	__svgalib_outGR(0x0D, regs[CIRRUSREG_GR(0x0D)]);
    }
    if (CHIP_HAS_GRE())
	__svgalib_outGR(0x0E, regs[CIRRUSREG_GR(0x0E)]);
    if (CHIP_HAS_GR10_AND_GR11()) {
	__svgalib_outGR(0x10, regs[CIRRUSREG_GR(0x10)]);
	__svgalib_outGR(0x11, regs[CIRRUSREG_GR(0x11)]);
    }
    if (CHIP_HAS_BLT_REGISTERS()) {
	int i;

	for (i = 0x20; i <= 0x2A; i++)
	    __svgalib_outGR(i, regs[CIRRUSREG_GR(i)]);
	for (i = 0x2C; i < 0x2E; i++)
	    __svgalib_outGR(i, regs[CIRRUSREG_GR(i)]);
	if (CHIP_HAS_BLTWRITEMASK())
	    __svgalib_outGR(0x2D, regs[CIRRUSREG_GR(0x2D)]);
	__svgalib_outGR(0x30, regs[CIRRUSREG_GR(0x30)]);
	__svgalib_outGR(0x32, regs[CIRRUSREG_GR(0x32)]);
	if (CHIP_HAS_BLTTRANSPARENTCOLOR()) {
	    __svgalib_outGR(0x34, regs[CIRRUSREG_GR(0x34)]);
	    __svgalib_outGR(0x35, regs[CIRRUSREG_GR(0x35)]);
	    __svgalib_outGR(0x38, regs[CIRRUSREG_GR(0x38)]);
	    __svgalib_outGR(0x39, regs[CIRRUSREG_GR(0x39)]);
	}
    }
#endif

    /* Write Truecolor DAC register. */
    if (CHIP_HAS_HIDDENDAC())
	writehicolordac(regs[CIRRUS_HIDDENDAC]);

    /* Write extended sequencer registers. */

    /* Be careful to put the sequencer clocking mode in a safe state. */
    __svgalib_outSR(0x07, (regs[CIRRUS_SR7] & ~0x0F) | 0x01);

    __svgalib_outSR(0x0E, regs[CIRRUS_VCLK3NUMERATOR]);
    __svgalib_outSR(0x1E, regs[CIRRUS_VCLK3DENOMINATOR]);
    __svgalib_outSR(0x07, regs[CIRRUS_SR7]);
    __svgalib_outSR(0x0F, regs[CIRRUS_DRAMCONTROL]);
    if (CHIP_HAS_PERFTUNINGREGISTER())
	__svgalib_outSR(0x16, regs[CIRRUS_PERFTUNING]);
    if (CHIP_USE_SR17())
	__svgalib_outSR(0x17, regs[CIRRUS_SR17]);
    if (CHIP_HAS_MCLK_REGISTER())
	__svgalib_outSR(0x1F, regs[CIRRUS_MCLKREGISTER]);
}

#if 0
/* Return nonzero if mode is available */

static int cirrus_modeavailable(int mode)
{
    struct info *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    /* CLGD7548 mode 13 is broken. Remove this if when supported */
    if (mode == G320x200x256 && cirrus_chiptype == CLGD7548)
 return 0;


    info = &__svgalib_infotable[mode];
    if (cirrus_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }
    free(modetiming);
    free(modeinfo);

    return SVGADRV;
}
#endif


/* Set a mode */

/* Local, called by cirrus_setmode(). */

static void cirrus_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo)
{

    /* Get current values. */
    cirrus_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    /* Set up the extended register values, including modifications */
    /* of standard VGA registers. */

/* Graphics */
    moderegs[CIRRUS_GRAPHICSOFFSET1] = 0;	/* Reset banks. */
    moderegs[CIRRUS_GRAPHICSOFFSET2] = 0;
    moderegs[CIRRUS_GRB] = 0;	/* 0x01 enables dual banking. */
    if (cirrus_memory > 1024)
	/* Enable 16K granularity. */
	moderegs[CIRRUS_GRB] |= 0x20;
    moderegs[VGA_SR2] = 0xFF;	/* Plane mask */

/* CRTC */
    if (modetiming->VTotal >= 1024 && !(modetiming->flags & INTERLACED))
	/*
	 * Double the vertical timing. Used for 1280x1024 NI.
	 * The CrtcVTimings have already been adjusted
	 * by __svgalib_getmodetiming() because of the GREATER_1024_DIVIDE_VERT
	 * flag.
	 */
	moderegs[VGA_CR17] |= 0x04;
    moderegs[CIRRUS_CR1B] = 0x22;
    if (cirrus_chiptype >= CLGD5434)
	/* Clear display start bit 19. */
	SETBITS(moderegs[CIRRUS_CR1D], 0x80, 0);
    /* CRTC timing overflows. */
    moderegs[CIRRUS_CR1A] = 0;
    SETBITSFROMVALUE(moderegs[CIRRUS_CR1A], 0xC0,
		     modetiming->CrtcVSyncStart + 1, 0x300);
    SETBITSFROMVALUE(moderegs[CIRRUS_CR1A], 0x30,
		     modetiming->CrtcHSyncEnd, (0xC0 << 3));
    moderegs[CIRRUS_CR19] = 0;	/* Interlaced end. */
    if (modetiming->flags & INTERLACED) {
	moderegs[CIRRUS_CR19] =
	    ((modetiming->CrtcHTotal / 8) - 5) / 2;
	moderegs[CIRRUS_CR1A] |= 0x01;
    }
/* Scanline offset */
    if (modeinfo->bytesPerPixel == 4) {
	/* At 32bpp the chip does an extra multiplication by two. */
	if (cirrus_chiptype >= CLGD5436) {
		/* Do these chipsets multiply by 4? */
		moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 5;
		SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
				 modeinfo->lineWidth, 0x2000);
	}
	else {
		moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 4;
		SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
				 modeinfo->lineWidth, 0x1000);
	}
    } else if (modeinfo->bitsPerPixel == 4)
	/* 16 color mode (planar). */
	moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 1;
    else {
	moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 3;
	SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
			 modeinfo->lineWidth, 0x800);
    }

/* Clocking */
    moderegs[VGA_MISCOUTPUT] |= 0x0C;	/* Use VCLK3. */
    moderegs[CIRRUS_VCLK3NUMERATOR] =
	fixed_clock_numerator[modetiming->selectedClockNo];
    moderegs[CIRRUS_VCLK3DENOMINATOR] =
	fixed_clock_denominator[modetiming->selectedClockNo];

/* DAC register and Sequencer Mode */
    {
	unsigned char DAC, SR7;
	DAC = 0x00;
	SR7 = 0x00;
	if (modeinfo->bytesPerPixel > 0)
	    SR7 = 0x01;		/* Packed-pixel mode. */
	if (modeinfo->bytesPerPixel == 2) {
	    int rgbmode;
	    rgbmode = 0;	/* 5-5-5 RGB. */
	    if (modeinfo->colorBits == 16)
		rgbmode = 1;	/* Add one for 5-6-5 RGB. */
	    if (cirrus_chiptype >= CLGD5426) {
		/* Pixel clock (double edge) mode. */
		DAC = 0xD0 + rgbmode;
		SR7 = 0x07;
	    } else {
		/* Single-edge (double VCLK). */
		DAC = 0xF0 + rgbmode;
		SR7 = 0x03;
	    }
	}
	if (modeinfo->bytesPerPixel >= 3) {
	    /* Set 8-8-8 RGB mode. */
	    DAC = 0xE5;
	    SR7 = 0x05;
	    if (modeinfo->bytesPerPixel == 4)
		SR7 = 0x09;
	}
#ifdef SUPPORT_5434_PALETTE_CLOCK_DOUBLING
	if (modeinfo->bytesPerPixel == 1 && (modetiming->flags & HADJUSTED)) {
	    /* Palette clock doubling mode on 5434 8bpp. */
	    DAC = 0x4A;
	    SR7 = 0x07;
	}
#endif
	moderegs[CIRRUS_HIDDENDAC] = DAC;
	moderegs[CIRRUS_SR7] = SR7;
    }

/* DRAM control and CRT FIFO */
    if (cirrus_chiptype >= CLGD5422)
	/* Enable large CRT FIFO. */
	moderegs[CIRRUS_DRAMCONTROL] |= 0x20;
    if (cirrus_memory == 2048 && cirrus_chiptype <= CLGD5429)
	/* Enable DRAM Bank Select. */
	moderegs[CIRRUS_DRAMCONTROL] |= 0x80;
    if (cirrus_chiptype >= CLGD5424) {
	/* CRT FIFO threshold setting. */
	unsigned char threshold;
	threshold = 8;
/*	if (cirrus_chiptype >= CLGD5434)
	    threshold = 1;*/
	/* XXX Needs more elaborate setting. */
	SETBITS(moderegs[CIRRUS_PERFTUNING], 0x0F, threshold);
    }
    if (CHIP_HAS_MCLK_REGISTER())
	if (programmedMCLK != actualMCLK
	    && modeinfo->bytesPerPixel > 0)
	    /* Program higher MCLK for packed-pixel modes. */
	    moderegs[CIRRUS_MCLKREGISTER] = programmedMCLK;
}

/* Unlock chipset-specific registers */

static void cirrus_unlock(void)
{
    int vgaIOBase, temp;

    /* Are we Mono or Color? */
    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

    outb(SEQ_I, 0x06);
    outb(SEQ_D, 0x12);		/* unlock cirrus special */

    /* Put the Vert. Retrace End Reg in temp */

    outb(vgaIOBase + 4, 0x11);
    temp = inb(vgaIOBase + 5);

    /* Put it back with PR bit set to 0 */
    /* This unprotects the 0-7 CRTC regs so */
    /* they can be modified, i.e. we can set */
    /* the timing. */

    outb(vgaIOBase + 5, temp & 0x7F);
}


/* Relock chipset-specific registers */
/* (currently not used) */
// static void cirrus_lock(void)
// {
//     outb(SEQ_I, 0x06);
//     outb(SEQ_D, 0x0F);		/* relock cirrus special */
// }

int cirrus_test(void)
{
    int oldlockreg;
    int lockreg;

    outb(SEQ_I, 0x06);
    oldlockreg = inb(SEQ_D);

    cirrus_unlock();

    /* If it's a Cirrus at all, we should be */
    /* able to read back the lock register */

    outb(SEQ_I, 0x06);
    lockreg = inb(SEQ_D);

    /* Ok, if it's not 0x12, we're not a Cirrus542X. */
    if (lockreg != 0x12) {
	outb(SEQ_I, 0x06);
	outb(SEQ_D, oldlockreg);
	return 0;
    }
    /* The above check seems to be weak, so we also check the chip ID. */

    outb(__svgalib_CRT_I, 0x27);
    switch (inb(__svgalib_CRT_D) >> 2) {
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:			/* 5429 */
    case 0x28:			/* 5430 */
    case 0x2A:			/* 5434 */
    case 0x2B:			/* 5436 */
    case 0x2E:			/* 5446 */
    case 0x0E:			/* 7548 */
	break;
    default:
	outb(SEQ_I, 0x06);
	outb(SEQ_D, oldlockreg);
	return 0;
    }

    if (cirrus_init(0, 0, 0))
	return 0;		/* failure */
    return 1;
}

static int cirrus_init(int force, int par1, int par2)
{
    unsigned char v;
    cirrus_unlock();
    if (force) {
	cirrus_memory = par1;
	cirrus_chiptype = par2;
    } else {
	unsigned char partstatus;
	outb(__svgalib_CRT_I, 0x27);
	cirrus_chiptype = inb(__svgalib_CRT_D) >> 2;
	cirrus_chiprev = 0;
	partstatus = __svgalib_inCR(0x25);
	switch (cirrus_chiptype) {
	case 0x22:
	    cirrus_chiptype = CLGD5420;
#if 0
	    /* Check for CL-GD5420-75QC-B. */
	    /* It has a Hidden-DAC register. */
	    outb(0x3C6, 0x00);
	    outb(0x3C6, 0xFF);
	    inb(0x3C6);
	    inb(0x3c6);
	    inb(0x3C6);
	    inb(0x3C6);
	    if (inb(0x3C6) != 0xFF)
		cirrus_chiptype = CLGD5420B;
#endif
	    break;
	case 0x23:
	    cirrus_chiptype = CLGD5422;
	    break;
	case 0x24:
	    cirrus_chiptype = CLGD5426;
	    break;
	case 0x25:
	    cirrus_chiptype = CLGD5424;
	    /*
	     * Some CL-GD5422's ID as CL-GD5424.
	     * Check for writability of GRC.
	     */
	    v = __svgalib_inGR(0x0C);
	    __svgalib_outGR(0x0C, 0x55);
	    if (__svgalib_inGR(0x0C) != 0x55)
		cirrus_chiptype = CLGD5422;
	    __svgalib_outGR(0x0C, v);
	    break;
	case 0x26:
	    cirrus_chiptype = CLGD5428;
	    break;
	case 0x27:
	    cirrus_chiptype = CLGD5429;
	    break;
	case 0x28:
	    cirrus_chiptype = CLGD5430;
	    break;
	case 0x2A:
	    cirrus_chiptype = CLGD5434;
	    if ((partstatus & 0xC0) == 0xC0)
		/* Rev. E, can do 60 MHz MCLK. */
		cirrus_chiprev = 1;
	    break;
	case 0x2B:
	    cirrus_chiptype = CLGD5436;
	    break;
	case 0x2E:
	    /* Treat 5446 as 5436. */
	    cirrus_chiptype = CLGD5436;
	    break;
	case 0x0E:
	    cirrus_chiptype = CLGD7548;
	    break;
	default:
	    return -1;
	}

#if 0
	if (cirrus_chiptype == CLGD5422) {
	    /* Rev. C has programmable MCLK register; */
	    /* check for it. */
	    /* This check is wrong. */
	    if (__svgalib_inSR(0x1F) != 0xFF)
		cirrus_chiptype = CLGD5422C;
	}
#endif

	/* Now determine the amount of memory. */
	outb(SEQ_I, 0x0a);	/* read memory register */
	/* This depends on the BIOS having set a scratch register. */
	v = inb(SEQ_D);
	cirrus_memory = 256 << ((v >> 3) & 3);

	/* Determine memory the correct way for the 543x, and
	 * for the 542x if the amount seems incorrect. */
	if (cirrus_chiptype >= CLGD5430 || (cirrus_memory <= 256
				       && cirrus_chiptype != CLGD5420)) {
	    unsigned char SRF;
	    cirrus_memory = 512;
	    outb(SEQ_I, 0x0f);
	    SRF = inb(SEQ_D);
	    if (SRF & 0x10)
		/* 32-bit DRAM bus. */
		cirrus_memory *= 2;
	    if ((SRF & 0x18) == 0x18)
		/* 64-bit DRAM data bus width; assume 2MB. */
		/* Also indicates 2MB memory on the 5430. */
		cirrus_memory *= 2;
	    if (cirrus_chiptype != CLGD5430 && (SRF & 0x80))
		/* If DRAM bank switching is enabled, there */
		/* must be twice as much memory installed. */
		/* (4MB on the 5434) */
		cirrus_memory *= 2;
	}
    }
    if (CHIP_HAS_MCLK_REGISTER())
	actualMCLK = __svgalib_inSR(0x1F) & 0x3F;
    else {
	actualMCLK = 0;
        return -1;
    }
    programmedMCLK = actualMCLK;
    if (cirrus_chiptype == CLGD5434 && cirrus_chiprev > 0)
	/* 5434 rev. E+ supports 60 MHz in graphics modes. */
	programmedMCLK = 0x22;
    DRAMbandwidth = 14318 * (int) programmedMCLK / 16;
    if (cirrus_memory >= 512)
	/* At least 16-bit DRAM bus. */
	DRAMbandwidth *= 2;
    if (cirrus_memory >= 1024 && CHIP_HAS_32BIT_DRAM_BUS())
	/* At least 32-bit DRAM bus. */
	DRAMbandwidth *= 2;
#if 0
    /* we are not supporting 64-bit DRAM bus! */
    if (cirrus_memory >= 2048 && CHIP_HAS_64BIT_DRAM_BUS())
	/* 64-bit DRAM bus. */
	DRAMbandwidth *= 2;
#endif
    /*
     * Calculate highest acceptable DRAM bandwidth to be taken up
     * by screen refresh. Satisfies
     *     total bandwidth >= refresh bandwidth * 1.1
     */
    DRAMbandwidthLimit = (DRAMbandwidth * 10) / 11;

#if 0 /* WARNNING! */
    if(cirrus_chiptype>=CLGD5430) {
       int i;
        
       i=(__svgalib_inSR(0x17)>>3)&7;
       if(i==4) {
          unsigned long buf[64];
          int found, _ioperm=0;
         
          if(getenv("IOPERM")==NULL) {
             _ioperm=1;
             iopl(3);
          };
          found=__svgalib_pci_find_vendor_vga(0x1013,buf,0);
          if(_ioperm)iopl(0);
          if(!found)cirrus_pci_linear=buf[4]&0xffff0000;
       };
    };
#endif

/* begin: Initialize card specs. */
    cardspecs->videoMemory = cirrus_memory;
    /*
     * First determine clock limits for the chip (DAC), then
     * adjust them according to the available DRAM bandwidth.
     * For 32-bit DRAM cards the 16bpp clock limit is initially
     * set very high, but they are cut down by the DRAM bandwidth
     * check.
     */
    cardspecs->maxPixelClock4bpp = 75000;	/* 5420 */
    cardspecs->maxPixelClock8bpp = 45000;	/* 5420 */
    cardspecs->maxPixelClock16bpp = 0;	/* 5420 */
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 0;
    if (cirrus_chiptype == CLGD5420B) {
	/*
	 * CL-GD5420-75QC-B
	 * Deviating chip, may be used in cheap ISA cards.
	 * 32-bit DRAM bus and Truecolor DAC but cannot do
	 * LUT > 45 MHz, and maybe has less acceleration.
	 */
	cardspecs->maxPixelClock16bpp = 75000 / 2;
	cardspecs->maxPixelClock24bpp = 25175;
    }
    if (cirrus_chiptype >= CLGD5422) {
	/* 5422/24/26/28 have VCLK spec of 80 MHz. */
	cardspecs->maxPixelClock4bpp = 80000;
	cardspecs->maxPixelClock8bpp = 80000;
	if (cirrus_chiptype >= CLGD5426)
	    /* DRAM bandwidth will be limiting factor. */
	    cardspecs->maxPixelClock16bpp = 80000;
	else
	    /* Clock / 2 16bpp requires 32-bit DRAM bus. */
	if (cirrus_memory >= 1024)
	    cardspecs->maxPixelClock16bpp = 80000 / 2;
	/* Clock / 3 24bpp requires 32-bit DRAM bus. */
	if (cirrus_memory >= 1024)
	    cardspecs->maxPixelClock24bpp = 80000 / 3;
    }
    if (cirrus_chiptype >= CLGD5429) {
	/* 5429, 5430, 5434 have VCLK spec of 86 MHz. */
	cardspecs->maxPixelClock4bpp = 86000;
	cardspecs->maxPixelClock8bpp = 86000;
	cardspecs->maxPixelClock16bpp = 86000;
	if (cirrus_memory >= 1024)
	    cardspecs->maxPixelClock24bpp = 86000 / 3;
    }
    if (cirrus_chiptype == CLGD5434) {
#ifdef SUPPORT_5434_PALETTE_CLOCK_DOUBLING
	cardspecs->maxPixelClock8bpp = 108300;
	if (cirrus_chiprev > 0)
	    /* 5434 rev E+ */
	    cardspecs->maxPixelClock8bpp = 135300;
#endif
	if (cirrus_memory >= 2048)
	    /* 32bpp requires 64-bit DRAM bus. */
	    cardspecs->maxPixelClock32bpp = 86000;
    }
    if (cirrus_chiptype >=CLGD5436) {
#ifdef SUPPORT_5434_PALETTE_CLOCK_DOUBLING
        cardspecs->maxPixelClock8bpp = 135300;
#endif
	if (cirrus_memory >= 2048)
	    /* 32bpp requires 64-bit DRAM bus. */
	    cardspecs->maxPixelClock32bpp = 86000;
    }
        
    cardspecs->maxPixelClock8bpp = min(cardspecs->maxPixelClock8bpp,
				       DRAMbandwidthLimit);
    cardspecs->maxPixelClock16bpp = min(cardspecs->maxPixelClock16bpp,
					DRAMbandwidthLimit / 2);
    cardspecs->maxPixelClock24bpp = min(cardspecs->maxPixelClock24bpp,
					DRAMbandwidthLimit / 3);
    cardspecs->maxPixelClock32bpp = min(cardspecs->maxPixelClock32bpp,
					DRAMbandwidthLimit / 4);
    cardspecs->flags = INTERLACE_DIVIDE_VERT | GREATER_1024_DIVIDE_VERT;
    /* Initialize clocks (only fixed set for now). */
    cardspecs->nClocks = NU_FIXED_CLOCKS;

    cardspecs->clocks = cirrus_fixed_clocks;
#if 0 /* WARNNING */
    cardspecs->mapClock = cirrus_map_clock;
    cardspecs->mapHorizontalCrtc = cirrus_map_horizontal_crtc;
#endif
    cardspecs->maxHorizontalCrtc = 2040;
    /* Disable 16-color SVGA modes (don't work correctly). */
    cardspecs->maxPixelClock4bpp = 0;
/* end: Initialize card specs. */

#if 0
/* begin: Initialize driver options. */
    register_option(5434 _VLB_ZERO_WAITSTATE);
    register_option(50 MHZ_MCLK);
    register_option(55 MHZ_MCLK);
    register_option(60 MHZ_MCLK);
    register_option(542 X_VLB_NORMAL_WAITSTATES);
    register_option(CRT_FIFO_CONSERVATIVE);
    register_option(CRT_FIFO_AGGRESSIVE);
    register_option(NO_ACCELERATION);
    register_option(NO_MMIO);
/* end: Initialize driver options. */
#endif

/* Initialize accelspecs structure. */
    __svgalib_banked_mem_base=0xa0000;
	/* We set a output video page as 64k */
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=cirrus_pci_linear;
    __svgalib_linear_mem_size=cirrus_memory*0x400;
    __svgalib_mmio_size=32768;
    __svgalib_mmio_base=0xb8000;

    MMIO_POINTER = (unsigned char *) __svgalib_mmio_base;

    return 0;
}


static info_t VGA_G640x480x32K = {640, 480, 1 << 15, 640 * 2, 2};
static info_t VGA_G800x600x32K = {800, 600, 1 << 15, 800 * 2, 2};
static info_t VGA_G1024x768x32K = {1024, 768, 1 << 15, 1024 * 2, 2};


int cirrus_setmode(int mode, int prv_mode) {

	switch (mode)
	{
	case G640x480x32K:
		cur_vga_info = &VGA_G640x480x32K;
		break;
	case G800x600x32K:
		cur_vga_info = &VGA_G800x600x32K;
		break;
	case G1024x768x32K:
		cur_vga_info = &VGA_G1024x768x32K;
		break;
	default:
		return -1;
	}
	
    ModeTiming __modetiming;
    unsigned char moderegs[CIRRUS_TOTAL_REGS];
    ModeTiming *modetiming = &__modetiming;
    ModeInfo *modeinfo;

#if 0
    if (!cirrus_modeavailable(mode))
	return 1;
#endif

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(cur_vga_info);

    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	return 1;
    }

    cirrus_initializemode(moderegs, modetiming, modeinfo);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    cirrus_setregs(moderegs, mode);	/* Set extended regs. */

    __svgalib_InitializeAcceleratorInterface(modeinfo);

    __svgalib_cirrusaccel_init(NULL, (int)(modeinfo->bitsPerPixel),
		modeinfo->lineWidth / modeinfo->bytesPerPixel);

    return 0;
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

void cirrus_setdisplaystart(int address)
{
    outw(CRT_IC, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
    outw(CRT_IC, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    inb(0x3da);			/* set ATC to addressing mode */
    outb(ATT_IW, 0x13 + 0x20);	/* select ATC reg 0x13 */
    /* Cirrus specific bits 0,1 and 18,19,20: */
    outb(ATT_IW, (inb(ATT_R) & 0xf0) | (address & 3));
    /* write sa0-1 to bits 0-1; other cards use bits 1-2 */
    outb(CRT_IC, 0x1b);
    outb(CRT_DC, (inb(CRT_DC) & 0xf2)
	 | ((address & 0x40000) >> 18)	/* sa18: write to bit 0 */
	 |((address & 0x80000) >> 17)	/* sa19: write to bit 2 */
	 |((address & 0x100000) >> 17));	/* sa20: write to bit 3 */
    outb(CRT_IC, 0x1d);
    if (cirrus_memory > 2048)
	outb(CRT_DC, (inb(CRT_DC) & 0x7f)
	     | ((address & 0x200000) >> 14));	/* sa21: write to bit 7 */
}

void cirrus_setlogicalwidth(int width)
{
    outw(CRT_IC, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
    outb(CRT_IC, 0x1b);
    outb(CRT_DC, (inb(CRT_DC) & 0xef) | ((width & 0x800) >> 7));
    /* write lw12 to bit 4 of Sequencer reg. 0x1b */
}

void cirrus_setlinear(int addr)
{
    int val;
    outb(SEQ_I, 0x07);
    val = inb(SEQ_D);
    outb(SEQ_D, (val & 0x0f) | (addr << 4));
}


/*      Some information on the accelerated features of the 5426,
   derived from the Databook.

   port index

   Addresses have 21 bits (2Mb of memory).
   GRA_I,       0x28    bits 0-7 of the destination address
   GRA_I,       0x29    bits 8-15
   GRA_I,       0x2a    bits 16-20

   GRA_I,       0x2c    bits 0-7 of the source address
   GRA_I,       0x2d    bits 8-15
   GRA_I,       0x2e    bits 16-20

   Maximum pitch is 4095.
   GRA_I,       0x24    bits 0-7 of the destination pitch (screen width)
   GRA_I,       0x25    bits 8-11

   GRA_I,       0x26    bits 0-7 of the source pitch (screen width)
   GRA_I,       0x27    bits 8-11

   Maximum width is 2047.
   GRA_I,       0x20    bits 0-7 of the box width - 1
   GRA_I,       0x21    bits 8-10

   Maximum height is 1023.
   GRA_I,       0x22    bits 0-7 of the box height - 1
   GRA_I,       0x23    bits 8-9

   GRA_I,       0x30    BLT mode
   bit 0: direction (0 = down, 1 = up)
   bit 1: destination
   bit 2: source (0 = video memory, 1 = system memory)
   bit 3: enable transparency compare
   bit 4: 16-bit color expand/transparency
   bit 6: 8x8 pattern copy
   bit 7: enable color expand

   0x31 BLT status
   bit 0: busy
   bit 1: start operation (1)/suspend (0)
   bit 2: reset
   bit 3: set while blit busy/suspended

   0x32 BLT raster operation
   0x00 black
   0x01 white
   0x0d copy source
   0xd0 copy inverted source
   0x0b invert destination
   0x05 logical AND
   0x6d logical OR (paint)
   0x59 XOR

   0x34 BLT transparent color
   0x35 high byte

   GRA_I,  0x00 background color (for color expansion)
   GRA_I,  0x01 foreground color
   GRA_I,  0x10 high byte of background color for 16-bit pixels
   GRA_I,  0x11 high byte of foreground color

   GRA_I,       0x0b    bit 1: enable BY8 addressing
   SEQ_I,       0x02    8-bit plane mask for BY8 (corresponds to 8 pixels)
   (write mode 1, 4, 5)
   SEQ_D,  0x05 bits 0-2: VGA write mode
   extended write mode 4: write up to 8 pixels in
   foreground color (BY8)
   extended write mode 5: write 8 pixels in foreground/
   background color (BY8)
   This may also work in normal non-BY8 packed-pixel mode.

   When doing blits from system memory to video memory, pixel data
   can apparently be written to any video address in 16-bit words, with
   the each scanline padded to 4-byte alignment. This is handy because
   the chip handles line transitions and alignment automatically (and
   can do, for example, masking).

   The pattern copy requires an 8x8 pattern (64 pixels) at the source
   address in video memory, and fills a box with specified size and 
   destination address with the pattern. This is in fact the way to do
   solid fills.

   mode                 pattern
   Color Expansion              8 bytes (monochrome bitmap)
   8-bit pixels         64 bytes
   16-bit pixels                128 bytes

 */

/* BitBLT modes. */

#define FORWARDS		0x00
#define BACKWARDS		0x01
#define SYSTEMDEST		0x02
#define SYSTEMSRC		0x04
#define TRANSPARENCYCOMPARE	0x08
#define PIXELWIDTH16		0x10
#define PIXELWIDTH32		0x30	/* 543x only. */
#define PATTERNCOPY		0x40
#define COLOREXPAND		0x80

/* Macros for normal I/O BitBLT register access. */

#define SETSRCADDR(addr) \
	outw(GRA_I, (((addr) & 0x000000FF) << 8) | 0x2C); \
	outw(GRA_I, (((addr) & 0x0000FF00)) | 0x2D); \
	outw(GRA_I, (((addr) & 0x003F0000) >> 8) | 0x2E);

#define SETDESTADDR(addr) \
	outw(GRA_I, (((addr) & 0x000000FF) << 8) | 0x28); \
	outw(GRA_I, (((addr) & 0x0000FF00)) | 0x29); \
	outw(GRA_I, (((addr) & 0x003F0000) >> 8) | 0x2A);

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define SETDESTPITCH(pitch) \
	outw(GRA_I, (((pitch) & 0x000000FF) << 8) | 0x24); \
	outw(GRA_I, (((pitch) & 0x00001F00)) | 0x25);

#define SETSRCPITCH(pitch) \
	outw(GRA_I, (((pitch) & 0x000000FF) << 8) | 0x26); \
	outw(GRA_I, (((pitch) & 0x00001F00)) | 0x27);

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define SETWIDTH(width) \
	outw(GRA_I, ((((width) - 1) & 0x000000FF) << 8) | 0x20); \
	outw(GRA_I, ((((width) - 1) & 0x00001F00)) | 0x21);

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */
/* It appears many 5434's only go up to 1024. */

#define SETHEIGHT(height) \
	outw(GRA_I, ((((height) - 1) & 0x000000FF) << 8) | 0x22); \
	outw(GRA_I, (((height) - 1) & 0x00000700) | 0x23);

#define SETBLTMODE(m) \
	outw(GRA_I, ((m) << 8) | 0x30);

#define SETBLTWRITEMASK(m) \
	outw(GRA_I, ((m) << 8) | 0x2F);

#define SETTRANSPARENCYCOLOR(c) \
	outw(GRA_I, ((c) << 8) | 0x34);

#define SETTRANSPARENCYCOLOR16(c) \
	outw(GRA_I, ((c) << 8) | 0x34); \
	outw(GRA_I, (c & 0xFF00) | 0x35);

#define SETTRANSPARENCYCOLORMASK16(m) \
	outw(GRA_I, ((m) << 8) | 0x38); \
	outw(GRA_I, ((m) & 0xFF00) | 0x39);

#define SETROP(rop) \
	outw(GRA_I, ((rop) << 8) | 0x32);

#define SETFOREGROUNDCOLOR(c) \
	outw(GRA_I, 0x01 + ((c) << 8));

#define SETBACKGROUNDCOLOR(c) \
	outw(GRA_I, 0x00 + ((c) << 8));

#define SETFOREGROUNDCOLOR16(c) \
	outw(GRA_I, 0x01 + ((c) << 8)); \
	outw(GRA_I, 0x11 + ((c) & 0xFF00));

#define SETBACKGROUNDCOLOR16(c) \
	outw(GRA_I, 0x00 + ((c) << 8)); \
	outw(GRA_I, 0x10 + ((c) & 0xFF00)); \

#define SETFOREGROUNDCOLOR32(c) \
	outw(GRA_I, 0x01 + ((c) << 8)); \
	outw(GRA_I, 0x11 + ((c) & 0xFF00)); \
	outw(GRA_I, 0x13 + (((c) & 0xFf0000) >> 8)); \
	outw(GRA_I, 0x15 + (((unsigned int)(c) & 0xFF000000) >> 16));

#define SETBACKGROUNDCOLOR32(c) \
	outw(GRA_I, 0x00 + ((c) << 8)); \
	outw(GRA_I, 0x10 + ((c) & 0xFF00)); \
	outw(GRA_I, 0x12 + (((c) & 0xFF0000) >> 8)); \
	outw(GRA_I, 0x14 + (((unsigned int)(c) & 0xFF000000) >> 16));

#define STARTBLT() { \
	unsigned char tmp; \
	outb(GRA_I, 0x31); \
	tmp = inb(GRA_D); \
	outb(GRA_D, tmp | 0x02); \
	}

#define BLTBUSY(s) { \
	outb(GRA_I, 0x31); \
	s = inb(GRA_D) & 1; \
	}

#define WAITUNTILFINISHED() \
	for (;;) { \
		int busy; \
		BLTBUSY(busy); \
		if (!busy) \
			break; \
	}


/* Macros for memory-mapped I/O BitBLT register access. */

/* MMIO addresses (offset from 0xb8000). */

#define MMIOBACKGROUNDCOLOR	0x00
#define MMIOFOREGROUNDCOLOR	0x04
#define MMIOWIDTH		0x08
#define MMIOHEIGHT		0x0A
#define MMIODESTPITCH		0x0C
#define MMIOSRCPITCH		0x0E
#define MMIODESTADDR		0x10
#define MMIOSRCADDR		0x14
#define MMIOBLTWRITEMASK	0x17
#define MMIOBLTMODE		0x18
#define MMIOROP			0x1A
#define MMIOBLTSTATUS		0x40

#define MMIOSETDESTADDR(addr) \
  *(unsigned int *)(MMIO_POINTER + MMIODESTADDR) = addr;

#define MMIOSETSRCADDR(addr) \
  *(unsigned int *)(MMIO_POINTER + MMIOSRCADDR) = addr;

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define MMIOSETDESTPITCH(pitch) \
  *(unsigned short *)(MMIO_POINTER + MMIODESTPITCH) = pitch;

#define MMIOSETSRCPITCH(pitch) \
  *(unsigned short *)(MMIO_POINTER + MMIOSRCPITCH) = pitch;

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define MMIOSETWIDTH(width) \
  *(unsigned short *)(MMIO_POINTER + MMIOWIDTH) = (width) - 1;

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */

#define MMIOSETHEIGHT(height) \
  *(unsigned short *)(MMIO_POINTER + MMIOHEIGHT) = (height) - 1;

#define MMIOSETBLTMODE(m) \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTMODE) = m;

#define MMIOSETBLTWRITEMASK(m) \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTWRITEMASK) = m;

#define MMIOSETROP(rop) \
  *(unsigned char *)(MMIO_POINTER + MMIOROP) = rop;

#define MMIOSTARTBLT() \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTSTATUS) |= 0x02;

#define MMIOBLTBUSY(s) \
  s = *(volatile unsigned char *)(MMIO_POINTER + MMIOBLTSTATUS) & 1;

#define MMIOSETBACKGROUNDCOLOR(c) \
  *(unsigned char *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR(c) \
  *(unsigned char *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOSETBACKGROUNDCOLOR16(c) \
  *(unsigned short *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR16(c) \
  *(unsigned short *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOSETBACKGROUNDCOLOR32(c) \
  *(unsigned int *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR32(c) \
  *(unsigned int *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOWAITUNTILFINISHED() \
	for (;;) { \
		int busy; \
		MMIOBLTBUSY(busy); \
		if (!busy) \
			break; \
	}

static int cirrus_pattern_address;	/* Pattern with 1's (8 bytes) */
static int cirrus_bitblt_pixelwidth;
/* Foreground color is not preserved on 5420/2/4/6/8. */
static int cirrus_accel_foreground_color;

void __svgalib_cirrusaccel_init(void* accelspecs, int bpp, int width_in_pixels)
{
    /* [Setup accelerator screen pitch] */
    /* [Prepare any required off-screen space] */
    if (cirrus_chiptype < CLGD5426)
	/* No BitBLT engine. */
	return;
    if (bpp == 8)
	cirrus_bitblt_pixelwidth = 0;
    if (bpp == 16)
	cirrus_bitblt_pixelwidth = PIXELWIDTH16;
    if (bpp == 32)
	cirrus_bitblt_pixelwidth = PIXELWIDTH32;
    SETSRCPITCH(__svgalib_accel_screenpitchinbytes);
    SETDESTPITCH(__svgalib_accel_screenpitchinbytes);
    SETROP(0x0D);
    cirrus_pattern_address = cirrus_memory * 1024 - 8;
    vga_setpage(cirrus_pattern_address / 65536);
    gr_writel(0xffffffff, cirrus_pattern_address & 0xffff);
    gr_writel(0xffffffff, (cirrus_pattern_address & 0xffff) + 4);
    vga_setpage(0);
    if (cirrus_chiptype >= CLGD5429)
	/* Enable memory-mapped I/O. */
	__svgalib_outSR(0x17, __svgalib_inSR(0x17) | 0x04);
}


/*
 * Note: The foreground color register must always be reset to 0
 * on the 542x to avoid problems in normal framebuffer operation.
 * This is not the case on chips that support memory-mapped I/O.
 */

/*
 * These are two auxilliary functions to program the foreground
 * color depending on the current depth.
 */

static void set_foreground_color(int fg)
{
    if (__svgalib_accel_bytesperpixel == 1) {
	SETFOREGROUNDCOLOR(fg);
	return;
    }
    if (__svgalib_accel_bytesperpixel == 2) {
	SETFOREGROUNDCOLOR16(fg);
	return;
    }
    SETFOREGROUNDCOLOR32(fg);
}

static void mmio_set_foreground_color(int fg)
{
    if (__svgalib_accel_bytesperpixel == 1) {
	MMIOSETFOREGROUNDCOLOR(fg);
	return;
    }
    if (__svgalib_accel_bytesperpixel == 2) {
	MMIOSETFOREGROUNDCOLOR16(fg);
	return;
    }
    MMIOSETFOREGROUNDCOLOR32(fg);
}

#define FINISHBACKGROUNDBLITS() \
	if (__svgalib_accel_mode & BLITS_IN_BACKGROUND) \
		WAITUNTILFINISHED();

#define MMIOFINISHBACKGROUNDBLITS() \
	if (__svgalib_accel_mode & BLITS_IN_BACKGROUND) \
		MMIOWAITUNTILFINISHED();

void __svgalib_cirrusaccel_FillBox(int x, int y, int width, int height)
{
    int destaddr;
    destaddr = BLTBYTEADDRESS(x, y);
    width *= __svgalib_accel_bytesperpixel;
    FINISHBACKGROUNDBLITS();
    SETSRCADDR(cirrus_pattern_address);
    SETDESTADDR(destaddr);
    SETWIDTH(width);
    SETHEIGHT(height);
    set_foreground_color(cirrus_accel_foreground_color);
    SETBLTMODE(COLOREXPAND | PATTERNCOPY | cirrus_bitblt_pixelwidth);
    STARTBLT();
    WAITUNTILFINISHED();
    /* Can't easily run in background because foreground color has */
    /* to be restored. */
    SETFOREGROUNDCOLOR(0x00);
}

void __svgalib_cirrusaccel_mmio_FillBox(int x, int y, int width, int height)
{
    int destaddr;
    destaddr = BLTBYTEADDRESS(x, y);
    width *= __svgalib_accel_bytesperpixel;
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETSRCADDR(cirrus_pattern_address);
    MMIOSETDESTADDR(destaddr);
    MMIOSETWIDTH(width);
    MMIOSETHEIGHT(height);
    MMIOSETBLTMODE(COLOREXPAND | PATTERNCOPY | cirrus_bitblt_pixelwidth);
    MMIOSTARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	MMIOWAITUNTILFINISHED();
}

void __svgalib_cirrusaccel_ScreenCopy(int x1, int y1, int x2, int y2, int width,
			    int height)
{
    int srcaddr, destaddr, dir;
    width *= __svgalib_accel_bytesperpixel;
    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    dir = FORWARDS;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
	&& y1 + height > y2) {
	srcaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
	destaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
	dir = BACKWARDS;
    }
    FINISHBACKGROUNDBLITS();
    SETSRCADDR(srcaddr);
    SETDESTADDR(destaddr);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETBLTMODE(dir);
    STARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_cirrusaccel_mmio_ScreenCopy(int x1, int y1, int x2, int y2, int width,
				 int height)
{
    int srcaddr, destaddr, dir;
    width *= __svgalib_accel_bytesperpixel;
    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    dir = FORWARDS;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
	&& y1 + height > y2) {
	srcaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
	destaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
	dir = BACKWARDS;
    }
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETSRCADDR(srcaddr);
    MMIOSETDESTADDR(destaddr);
    MMIOSETWIDTH(width);
    MMIOSETHEIGHT(height);
    /* WARNING! */
    MMIOSETBLTMODE(dir);
    MMIOSTARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	MMIOWAITUNTILFINISHED();
}

void __svgalib_cirrusaccel_SetFGColor(int fg)
{
    cirrus_accel_foreground_color = fg;
}

void __svgalib_cirrusaccel_mmio_SetFGColor(int fg)
{
    MMIOFINISHBACKGROUNDBLITS();
    mmio_set_foreground_color(fg);
}

static unsigned char cirrus_rop_map[] =
{
    0x0D,			/* ROP_COPY */
    0x6D,			/* ROP_OR */
    0x05,			/* ROP_AND */
    0x59,			/* ROP_XOR */
    0x0B			/* ROP_INVERT */
};

void __svgalib_cirrusaccel_SetRasterOp(int rop)
{
    FINISHBACKGROUNDBLITS();
    SETROP(cirrus_rop_map[rop]);
}

void __svgalib_cirrusaccel_mmio_SetRasterOp(int rop)
{
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETROP(cirrus_rop_map[rop]);
}

void __svgalib_cirrusaccel_SetTransparency(int mode, int color)
{
    FINISHBACKGROUNDBLITS();
    if (mode == DISABLE_TRANSPARENCY_COLOR) {
	/* Disable. */
	SETTRANSPARENCYCOLORMASK16(0xFFFF);
	return;
    }
    if (mode == ENABLE_TRANSPARENCY_COLOR) {
	if (__svgalib_accel_bytesperpixel == 1)
	    color += color << 8;
	SETTRANSPARENCYCOLORMASK16(0x0000);
	SETTRANSPARENCYCOLOR16(color);
	return;
    }
    if (mode == DISABLE_BITMAP_TRANSPARENCY) {
	__svgalib_accel_bitmaptransparency = 0;
	return;
    }
    /* mode == ENABLE_BITMAP_TRANSPARENCY */
    __svgalib_accel_bitmaptransparency = 1;
}

void __svgalib_cirrusaccel_Sync(void)
{
    WAITUNTILFINISHED();
}

void __svgalib_cirrusaccel_mmio_Sync(void)
{
    MMIOWAITUNTILFINISHED();
}
