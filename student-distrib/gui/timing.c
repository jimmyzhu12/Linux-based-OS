/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * timing.c
 * 
 * tab:8
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      timing.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Adaptaed from timing.c of SVGAlib 1.4.3. Final revision.
 *        Fix all warnings.
 * 
 */

#include "../macros.h"
#include "timing.h"		/* Types. */
#include "macros.h"


/* 640x480 at 60 Hz, 31.5 kHz hsync */
MonitorModeTiming MMT_640_480_60HZ =
	{25175, 640, 664, 760, 800, 480, 491, 493, 525, 0};

/* 800x600 at 60 Hz, 37.8 kHz hsync */
MonitorModeTiming MMT_800_600_60HZ =
	{40000, 800, 840, 968, 1056, 600, 601, 605, 628, PHSYNC | PVSYNC};

/* 1024x768 at 60 Hz, 48.4 kHz hsync */
MonitorModeTiming MMT_1024_768_60HZ =
	{65000, 1024, 1032, 1176, 1344, 768, 771, 777, 806, NHSYNC | NVSYNC};

static MonitorModeTiming *current_timing;

/*
 * SYNC_ALLOWANCE is in percent
 * 1% corresponds to a 315 Hz deviation at 31.5 kHz, 1 Hz at 100 Hz
 */
#define SYNC_ALLOWANCE 1

#define INRANGE(x,y) \
    ((x) > __svgalib_##y.min * (1.0f - SYNC_ALLOWANCE / 100.0f) && \
     (x) < __svgalib_##y.max * (1.0f + SYNC_ALLOWANCE / 100.0f))

/*
 * The __svgalib_getmodetiming function looks up a mode in the standard mode
 * timings, choosing the mode with the highest dot clock that matches
 * the requested svgalib mode, and is supported by the hardware
 * (card limits, and monitor type). cardlimits points to a structure
 * of type CardSpecs that describes the dot clocks the card supports
 * at different depths. Returns non-zero if no mode is found.
 */

/*
 * findclock is an auxilliary function that checks if a close enough
 * pixel clock is provided by the card. Returns clock number if
 * succesful (a special number if a programmable clock must be used), -1
 * otherwise.
 */

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs * cardspecs)
{
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
	int diff;
	diff = cardspecs->clocks[i] - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return i;
    }
    /* Dont't try programmable clocks! */
    /* No close enough clock found. */
    return -1;
}

int __svgalib_getmodetiming(ModeTiming * modetiming, ModeInfo * modeinfo,
		  CardSpecs * cardspecs)
{
    int maxclock;
    MonitorModeTiming *besttiming;

    switch (modeinfo->height)
    {
    case 480:
	besttiming = &MMT_640_480_60HZ;
	break;
    case 600:
	besttiming = &MMT_800_600_60HZ;
	break;
    case 768:
	besttiming = &MMT_1024_768_60HZ;
	break;
    default:
	besttiming = NULL;
    }

    /* Get the maximum pixel clock for the depth of the requested mode. */
    if (modeinfo->bitsPerPixel == 4)
	maxclock = cardspecs->maxPixelClock4bpp;
    else if (modeinfo->bitsPerPixel == 8)
	maxclock = cardspecs->maxPixelClock8bpp;
    else if (modeinfo->bitsPerPixel == 16) {
	if ((cardspecs->flags & NO_RGB16_565)
	    && modeinfo->greenWeight == 6)
	    return 1;		/* No 5-6-5 RGB. */
	maxclock = cardspecs->maxPixelClock16bpp;
    } else if (modeinfo->bitsPerPixel == 24)
	maxclock = cardspecs->maxPixelClock24bpp;
    else if (modeinfo->bitsPerPixel == 32)
	maxclock = cardspecs->maxPixelClock32bpp;
    else
	maxclock = 0;

    /*
     * Copy the selected timings into the result, which may
     * be adjusted for the chipset.
     */

    modetiming->flags = besttiming->flags;
    modetiming->pixelClock = besttiming->pixelClock;	/* Formal clock. */

    /*
     * We know a close enough clock is available; the following is the
     * exact clock that fits the mode. This is probably different
     * from the best matching clock that will be programmed.
     */

    /* Fill in the best-matching clock that will be programmed. */
    modetiming->selectedClockNo = findclock(besttiming->pixelClock, cardspecs);
    if (modetiming->selectedClockNo == PROGRAMMABLE_CLOCK_MAGIC_NUMBER) {
	return -1;
    } else {
	modetiming->programmedClock = cardspecs->clocks[
					    modetiming->selectedClockNo];
    }
    modetiming->HDisplay = besttiming->HDisplay;
    modetiming->HSyncStart = besttiming->HSyncStart;
    modetiming->HSyncEnd = besttiming->HSyncEnd;
    modetiming->HTotal = besttiming->HTotal;
    
    modetiming->CrtcHDisplay = besttiming->HDisplay;
    modetiming->CrtcHSyncStart = besttiming->HSyncStart;
    modetiming->CrtcHSyncEnd = besttiming->HSyncEnd;
    modetiming->CrtcHTotal = besttiming->HTotal;

    modetiming->VDisplay = besttiming->VDisplay;
    modetiming->VSyncStart = besttiming->VSyncStart;
    modetiming->VSyncEnd = besttiming->VSyncEnd;
    modetiming->VTotal = besttiming->VTotal;

    if (modetiming->flags & DOUBLESCAN){
	modetiming->VDisplay <<= 1;
	modetiming->VSyncStart <<= 1;
	modetiming->VSyncEnd <<= 1;
	modetiming->VTotal <<= 1;
    }
    modetiming->CrtcVDisplay = modetiming->VDisplay;
    modetiming->CrtcVSyncStart = modetiming->VSyncStart;
    modetiming->CrtcVSyncEnd = modetiming->VSyncEnd;
    modetiming->CrtcVTotal = modetiming->VTotal;
    if (((modetiming->flags & INTERLACED)
	 && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
	|| (modetiming->VTotal >= 1024
	    && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
	/*
	 * Card requires vertical CRTC timing to be halved for
	 * interlaced modes, or for all modes with vertical
	 * timing >= 1024.
	 */
	modetiming->CrtcVDisplay /= 2;
	modetiming->CrtcVSyncStart /= 2;
	modetiming->CrtcVSyncEnd /= 2;
	modetiming->CrtcVTotal /= 2;
	modetiming->flags |= VADJUSTED;
    }
    current_timing=besttiming;
    return 0;			/* Succesful. */
}
