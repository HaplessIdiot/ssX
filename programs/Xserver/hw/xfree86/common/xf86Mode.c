/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Mode.c,v 1.2 1998/07/25 16:55:10 dawes Exp $ */

/*
 * Copyright (c) 1997,1998 by The XFree86 Project, Inc.
 *
 * Authors: Dirk Hohndel <hohndel@XFree86.Org>
 *          David Dawes <dawes@XFree86.Org>
 *
 * This file includes helper functions for mode related things.
 */

#include "X.h"
#include "os.h"
#include "mibank.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * xf86GetNearestClock --
 *	Find closest clock to given frequency (in kHz).  This assumes the
 *	number of clocks is greater than zero.
 */
int
xf86GetNearestClock(ScrnInfoPtr scrp, int freq, Bool allowDiv2,
    int DivFactor, int MulFactor, int *divider)
{
    int nearestClock = 0;
    int minimumGap = abs(freq - scrp->clock[0]);
    int i, j, k;

    if (allowDiv2)
	k = 2;
    else
	k = 1;

    for (i = 0;  i < scrp->numClocks;  i++) {
	for (j = 1; j <= k; j++) {
	    int gap = abs((freq * j) - ((scrp->clock[i] * DivFactor) / MulFactor));
	    if (gap < minimumGap) {
	    	minimumGap = gap;
	    	nearestClock = i;
		if (divider != NULL)
		    *divider = (j - 1) * V_CLKDIV2;
	    }
	}
    }
    return nearestClock;
}

/*
 * xf86ModeStatusToString
 *
 * Convert a ModeStatus value to a printable message
 */

const char *
xf86ModeStatusToString(ModeStatus status)
{
    switch(status) {
    case MODE_OK:
	return "Mode OK";
    case MODE_HSYNC:
	return "hsync out of range";
    case MODE_VSYNC:
	return "vrefresh out of range";
    case MODE_H_ILLEGAL:
	return "illegal horizontal timings";
    case MODE_V_ILLEGAL:
	return "illegal vertical timings";
    case MODE_BAD_WIDTH:
	return "width requires unsupported line pitch";
    case MODE_NOMODE:
	return "no mode of this name";
    case MODE_NO_INTERLACE:
	return "interlace mode not supported";
    case MODE_NO_DBLESCAN:
	return "doublescan mode not supported";
    case MODE_MEM:
	return "insufficient memory for mode";
    case MODE_VIRTUAL_X:
	return "width too large for virtual size";
    case MODE_VIRTUAL_Y:
	return "height too large for virtual size";
    case MODE_MEM_VIRT:
	return "insufficient memory given virtual size";
    case MODE_NOCLOCK:
	return "no clock available for mode";
    case MODE_CLOCK_HIGH:
	return "mode clock too high";
    case MODE_CLOCK_LOW:
	return "mode clock too low";
    case MODE_CLOCK_RANGE:
	return "bad mode clock/interlace/doublescan";
    case MODE_BAD_HVALUE:
	return "horizontal timing out of range";
    case MODE_BAD_VVALUE:
	return "vertical timing out of range";
    case MODE_BAD:
	return "unknown reason";
    case MODE_ERROR:
	return "internal error";
    }
    return "unknown";
}

/*
 * xf86LookupMode
 *
 * This function returns a mode from the given list which matches the
 * given name.  When multiple modes with the same name are available,
 * the method of picking the matching mode is determined by the
 * strategy selected.  There is an implicit assumption in this that modes
 * with the same name are of the same size.
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    modep        pointer to the returned mode, which must have the name
 *                 field filled in.
 *    clockRanges  a list of clock ranges
 *    strategy     how to decide which mode to use from multiple modes with
 *                 the same name
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    modePool     the list of monitor modes compatible with the driver
 *    clocks       a list of discrete clocks
 *    numClocks    number of discrete clocks
 *    progClock    clock is programmable
 *
 * If a mode was found, its values are filled in to the area pointed to
 * by modep,  If a mode was not found the return value indicates the
 * reason.
 */

ModeStatus
xf86LookupMode(ScrnInfoPtr scrp, DisplayModePtr modep,
	       ClockRangePtr clockRanges, LookupModeFlags strategy)
{
    DisplayModePtr p, bestMode = NULL;
    ClockRangePtr cp;
    int i, k, gap, minimumGap = CLOCK_TOLERANCE + 1;
    double refresh, bestRefresh = 0.0;
    Bool found = FALSE;
    int extraFlags = 0;
    int clockIndex = -1;
    int MulFactor = 1;
    int DivFactor = 1;
    int ModePrivFlags = 0;
    ModeStatus status = MODE_NOMODE;
    Bool allowDiv2 = (strategy & LOOKUP_CLKDIV2) != 0;
    strategy &= ~LOOKUP_CLKDIV2;

    /* Some sanity checking */
    if (scrp == NULL || scrp->modePool == NULL ||
	(!scrp->progClock && scrp->numClocks == 0)) {
	ErrorF("xf86LookupMode: called with invalid scrnInfoRec\n");
	return MODE_ERROR;
    }
    if (modep == NULL || modep->name == NULL) {
	ErrorF("xf86LookupMode: called with invalid modep\n");
	return MODE_ERROR;
    }
    if (clockRanges == NULL) {
	ErrorF("xf86LookupMode: called with invalid clockRanges\n");
	return MODE_ERROR;
    }
    for (cp = clockRanges; cp != NULL; cp = cp->next) {
	/* DivFactor and MulFactor must be > 0 */
	cp->ClockDivFactor = max(1, cp->ClockDivFactor);
	cp->ClockMulFactor = max(1, cp->ClockMulFactor);
    }

    /* Scan the mode pool for matching names */
    for (p = scrp->modePool; p != NULL; p = p->next) {
	if (strcmp(p->name, modep->name) == 0) {
	    /* Check clock is in range */
	    for (cp = clockRanges; cp != NULL; cp = cp->next) {
		if ((cp->minClock <= p->Clock) &&
		    (cp->maxClock >= p->Clock) &&
		    (cp->interlaceAllowed || !(p->Flags & V_INTERLACE)) &&
		    (cp->doubleScanAllowed ||
		     (!(p->Flags & V_DBLSCAN)) && (p->VScan <= 1)))
		    break;
	    }
	    if (cp == NULL) {
		/*
		 * XXX Could do more here to provide a more detailed
		 * reason for not finding a mode.
		 */
		if (!found)
		    status = MODE_CLOCK_RANGE;
		continue;
	    }

	    /*
	     * If programmable clock and strategy is not LOOKUP_BEST_REFRESH,
	     * the required mode has been found, otherwise record the refresh
	     * and continue looking.
	     */
	    if (scrp->progClock) {
		found = TRUE;
		if (strategy != LOOKUP_BEST_REFRESH) {
		    bestMode = p;
		    DivFactor = cp->ClockDivFactor;
		    MulFactor = cp->ClockMulFactor;
		    ModePrivFlags = cp->PrivFlags;
		    break;
		} else {
		    refresh = p->Clock * 1000.0 / p->HTotal / p->VTotal;
		    if (p->Flags & V_INTERLACE) {
			refresh *= 2.0;
			refresh /= INTERLACE_REFRESH_WEIGHT;
		    }
		    if (p->Flags & V_DBLSCAN)
			refresh /= 2.0;
		    if (p->VScan > 1)
			refresh /= p->VScan;
		    if (refresh > bestRefresh) {
			bestMode = p;
			DivFactor = cp->ClockDivFactor;
			MulFactor = cp->ClockMulFactor;
			ModePrivFlags = cp->PrivFlags;
			bestRefresh = refresh;
		    }
		    continue;
		}
	    }

	    /*
	     * Clock is in range, so if it is not a programmable clock, find
	     * a matching clock.
	     */

	    i = xf86GetNearestClock(scrp, p->Clock, allowDiv2,
		cp->ClockDivFactor, cp->ClockMulFactor, &k);
	    /*
	     * If the clock is too far from the requested clock, this
	     * mode is no good.
	     */
	    if (k & V_CLKDIV2)
		gap = abs((p->Clock * 2) -
		    ((scrp->clock[i] * cp->ClockDivFactor) / cp->ClockMulFactor));
	    else
		gap = abs(p->Clock -
		    ((scrp->clock[i] * cp->ClockDivFactor) / cp->ClockMulFactor));
	    if (gap > minimumGap) {
		if (!found)
		    status = MODE_NOCLOCK;
		continue;
	    } else {
		found = TRUE;
	    }

	    /*
	     * If strategy is neither LOOKUP_BEST_REFRESH or
	     * LOOKUP_CLOSEST_CLOCK the required mode has been found.
	     */
	    if (strategy != LOOKUP_BEST_REFRESH &&
		strategy != LOOKUP_CLOSEST_CLOCK) {
		bestMode = p;
		DivFactor = cp->ClockDivFactor;
		MulFactor = cp->ClockMulFactor;
		ModePrivFlags = cp->PrivFlags;
		extraFlags = k;
		clockIndex = i;
		break;
	    } else
	    /* Otherwise, record the refresh or gap and continue looking */
	    if (strategy == LOOKUP_BEST_REFRESH) {
		refresh = p->Clock * 1000.0 / p->HTotal / p->VTotal;
		if (p->Flags & V_INTERLACE) {
		    refresh *= 2.0;
		    refresh /= INTERLACE_REFRESH_WEIGHT;
		}
		if (p->Flags & V_DBLSCAN)
		    refresh /= 2.0;
		if (p->VScan > 1)
		    refresh /= p->VScan;
		if (refresh > bestRefresh) {
		    bestMode = p;
		    DivFactor = cp->ClockDivFactor;
		    MulFactor = cp->ClockMulFactor;
		    ModePrivFlags = cp->PrivFlags;
		    extraFlags = k;
		    clockIndex = i;
		    bestRefresh = refresh;
		}
		continue;
	    } else if (strategy == LOOKUP_CLOSEST_CLOCK) {
		if (gap < minimumGap) {
		    bestMode = p;
		    DivFactor = cp->ClockDivFactor;
		    MulFactor = cp->ClockMulFactor;
		    ModePrivFlags = cp->PrivFlags;
		    extraFlags = k;
		    clockIndex = i;
		    minimumGap = gap;
		}
		continue;
	    }
	}
    }
    if (!found || bestMode == NULL)
	return status;

    /* Fill in the mode parameters */
    if (scrp->progClock) {
        modep->Clock		= bestMode->Clock;
	modep->ClockIndex	= -1;
	modep->SynthClock	= (modep->Clock * MulFactor) / DivFactor;
    } else {
	modep->Clock		= (scrp->clock[clockIndex] * DivFactor) / MulFactor;
	modep->ClockIndex	= clockIndex;
	modep->SynthClock	= scrp->clock[clockIndex];
	if (extraFlags & V_CLKDIV2) {
	    modep->Clock /= 2;
	    modep->SynthClock /= 2;
	}
    }
    modep->PrivFlags		= ModePrivFlags;
    modep->HDisplay		= bestMode->HDisplay;
    modep->HSyncStart		= bestMode->HSyncStart;
    modep->HSyncEnd		= bestMode->HSyncEnd;
    modep->HTotal		= bestMode->HTotal;
    modep->HSkew		= bestMode->HSkew;
    modep->VDisplay		= bestMode->VDisplay;
    modep->VSyncStart		= bestMode->VSyncStart;
    modep->VSyncEnd		= bestMode->VSyncEnd;
    modep->VTotal		= bestMode->VTotal;
    modep->VScan		= bestMode->VScan;
    modep->Flags		= bestMode->Flags | extraFlags;
    modep->CrtcHDisplay		= bestMode->CrtcHDisplay;
    modep->CrtcHSyncStart	= bestMode->CrtcHSyncStart;
    modep->CrtcHSyncEnd		= bestMode->CrtcHSyncEnd;
    modep->CrtcHTotal		= bestMode->CrtcHTotal;
    modep->CrtcHSkew		= bestMode->CrtcHSkew;
    modep->CrtcVDisplay		= bestMode->CrtcVDisplay;
    modep->CrtcVSyncStart	= bestMode->CrtcVSyncStart;
    modep->CrtcVSyncEnd		= bestMode->CrtcVSyncEnd;
    modep->CrtcVTotal		= bestMode->CrtcVTotal;
    modep->CrtcHAdjusted	= bestMode->CrtcHAdjusted;
    modep->CrtcVAdjusted	= bestMode->CrtcVAdjusted;

    return MODE_OK;
}

/*
 * xf86CheckModeForMonitor
 *
 * This function takes a mode and monitor description, and determines
 * if the mode is valid for the monitor.
 */

ModeStatus
xf86CheckModeForMonitor(DisplayModePtr mode, MonPtr monitor)
{
    int i;
    float hsync, vrefresh;

    /* Sanity checks */
    if (mode == NULL || monitor == NULL) {
	ErrorF("xf86CheckModeForMonitor: called with invalid parameters\n");
	return MODE_ERROR;
    }

    /* Some basic mode validity checks */
    if (0 >= mode->HDisplay || mode->HDisplay > mode->HSyncStart ||
	mode->HSyncStart >= mode->HSyncEnd || mode->HSyncEnd >= mode->HTotal)
	return MODE_H_ILLEGAL;

    if (0 >= mode->VDisplay || mode->VDisplay > mode->VSyncStart ||
	mode->VSyncStart >= mode->VSyncEnd || mode->VSyncEnd >= mode->VTotal)
	return MODE_V_ILLEGAL;

    /* Check hsync against the allowed ranges */
    hsync = (float)mode->Clock / (float)mode->HTotal;
    for (i = 0; i < monitor->nHsync; i++)
	if ((hsync > monitor->hsync[i].lo * (1.0 - SYNC_TOLERANCE)) &&
	    (hsync < monitor->hsync[i].hi * (1.0 + SYNC_TOLERANCE)))
	    break;

    /* Now see whether we ran out of sync ranges without finding a match */
    if (i == monitor->nHsync)
	return MODE_HSYNC;

    /* Check vrefresh against the allowed ranges */
    vrefresh = mode->Clock * 1000.0 / (mode->HTotal * mode->VTotal);
    if (mode->Flags & V_INTERLACE)
	vrefresh *= 2.0;
    if (mode->Flags & V_DBLSCAN)
	vrefresh /= 2.0;
    if (mode->VScan > 1)
	vrefresh /= (float)(mode->VScan);
    for (i = 0; i < monitor->nVrefresh; i++)
	if ((vrefresh > monitor->vrefresh[i].lo * (1.0 - SYNC_TOLERANCE)) &&
	    (vrefresh < monitor->vrefresh[i].hi * (1.0 + SYNC_TOLERANCE)))
	    break;

    /* Now see whether we ran out of refresh ranges without finding a match */
    if (i == monitor->nVrefresh)
	return MODE_VSYNC;

    /* Force interlaced modes to have an odd VTotal */
    if (mode->Flags & V_INTERLACE)
	mode->CrtcVTotal = mode->VTotal |= 1;

    return MODE_OK;
}

/*
 * xf86InitialCheckModeForDriver
 *
 * This function checks if a mode satisfies a driver's initial requirements:
 *   -  mode size fits within the available pixel area (memory)
 *   -  width lies within the range of supported line pitches
 *   -  mode size fits within virtual size (if fixed)
 *   -  horizontal timings are in range
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    mode         mode to check
 *    maxPitch     (optional) maximum line pitch
 *    virtualX     (optional) virtual width requested
 *    virtualY     (optional) virtual height requested
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    formats      pixel formats for screen
 *    numFormats   number of pixel formats
 *    fbFormat     pixel format for the framebuffer
 *    videoRam     video memory size (in kB)
 *    maxHValue    maximum horizontal timing value
 *    maxVValue    maximum vertical timing value
 */

ModeStatus
xf86InitialCheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode,
			      int maxPitch, int virtualX, int virtualY)
{
    /* Sanity checks */
    if (scrp == NULL || mode == NULL) {
	ErrorF("xf86InitialCheckModeForDriver: "
		"called with invalid parameters\n");
	return MODE_ERROR;
    }

    if (mode->HDisplay * mode->VDisplay * scrp->fbFormat.bitsPerPixel >
        scrp->videoRam * (1024 * 8))
        return MODE_MEM;

    if (maxPitch > 0 && mode->HDisplay > maxPitch)
	return MODE_BAD_WIDTH;

    if (virtualX > 0 && mode->HDisplay > virtualX)
	return MODE_VIRTUAL_X;

    if (virtualY > 0 && mode->VDisplay > virtualY)
	return MODE_VIRTUAL_Y;

    if (scrp->maxHValue > 0 && mode->HTotal > scrp->maxHValue)
	return MODE_BAD_HVALUE;

    if (scrp->maxVValue > 0 && mode->VTotal > scrp->maxVValue)
	return MODE_BAD_VVALUE;

    if (scrp->ValidMode)
	return scrp->ValidMode(scrp->scrnIndex, mode, FALSE, 0);

    /* Assume it is OK */
    return MODE_OK;
}

/*
 * xf86CheckModeForDriver
 *
 * This function is for checking modes while the server is running (for
 * use mainly by the VidMode extension).
 *
 * This function checks if a mode satisfies a driver's requirements:
 *   -  width lies within the line pitch
 *   -  mode size fits within virtual size
 *   -  horizontal/vertical timings are in range
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    mode         mode to check
 *    flags        not (currently) used
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    maxHValue    maximum horizontal timing value
 *    maxVValue    maximum vertical timing value
 *    virtualX     virtual width
 *    virtualY     virtual height
 *    clockRanges  allowable clock ranges
 */

ModeStatus
xf86CheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode, int flags)
{
/* To be implemented */
    return MODE_OK;
}

/*
 * xf86ValidateModes
 *
 * This function takes a set of mode names, modes and limiting conditions,
 * and selects a set of modes and parameters based on those conditions.
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    availModes   the list of modes available for the monitor
 *    modeNames    list of mode names that the screen is requesting
 *    clockRanges  a list of clock ranges
 *    linePitches  (optional) a list of line pitches
 *    minPitch     (optional) minimum line pitch (in pixels)
 *    maxPitch     (optional) maximum line pitch (in pixels)
 *    pitchInc     (optional) pitch increment (in bits)
 *    minHeight    (optional) minimum virtual height (in pixels)
 *    maxHeight    (optional) maximum virtual height (in pixels)
 *    virtualX     (optional) virtual width requested (in pixels)
 *    virtualY     (optional) virtual height requested (in pixels)
 *    apertureSize size of video aperture (in bytes)
 *    strategy     how to decide which mode to use from multiple modes with
 *                 the same name
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    clocks       a list of discrete clocks
 *    numClocks    number of discrete clocks
 *    progClock    clock is programmable
 *    formats      pixmap formats
 *    numFormats   number of pixmap formats
 *    fbFormat     format of the framebuffer
 *    videoRam     video memory size
 *    maxHValue    maximum horizontal timing value
 *    maxVValue    maximum vertical timing value
 *
 * The function fills in the following ScrnInfoRec fields:
 *    modePool     A subset of the modes available to the monitor which
 *		   are compatible with the driver.
 *    modes        one mode entry for each of the requested modes, with the
 *                 status field filled in to indicate if the mode has been
 *                 accepted or not.
 *    virtualX     the resulting virtual width
 *    virtualY     the resulting virtual height
 *    displayWidth the resulting line pitch
 *
 * The function's return value is the number of matching modes found, or -1
 * if an unrecoverable error was encountered.
 */

int
xf86ValidateModes(ScrnInfoPtr scrp, DisplayModePtr availModes,
		  char **modeNames, ClockRangePtr clockRanges,
		  int *linePitches, int minPitch, int maxPitch, int pitchInc,
		  int minHeight, int maxHeight, int virtualX, int virtualY,
		  int apertureSize, LookupModeFlags strategy)
{
    /* XXX Implement checking against minHeight and maxHeight */

    DisplayModePtr p, q, new;
    int i, numModes = 0;
    Bool firstMode = TRUE;
    ModeStatus status;
    int linePitch = -1, virtX = 0, virtY = 0;
    int newLinePitch, newVirtX, newVirtY;
    int pixelArea = scrp->videoRam * (1024 * 8);	/* in bits */
    int bitsPerPixel, pixmapPad;
    PixmapFormatRec * BankFormat;

    /* Some sanity checking */
    if (scrp == NULL || scrp->name == NULL ||
	(!scrp->progClock && scrp->numClocks == 0)) {
	ErrorF("xf86ValidateModes: called with invalid scrnInfoRec\n");
	return -1;
    }
    if (modeNames == NULL || modeNames[0] == NULL) {
	ErrorF("xf86ValidateModes: called with invalid modeNames\n");
	return -1;
    }
    if (linePitches != NULL && linePitches[0] <= 0) {
	ErrorF("xf86ValidateModes: called with invalid linePitches\n");
	return -1;
    }
    if (clockRanges == NULL) {
	ErrorF("xf86ValidateModes: called with invalid clockRanges\n");
	return -1;
    }
    if ((virtualX > 0) != (virtualY > 0)) {
	ErrorF("xf86ValidateModes: called with invalid virtual resolution\n");
	return -1;
    }

    /* Determine which pixmap format to pass to miScanLineWidth() */
    if (scrp->depth > 4)
	BankFormat = &scrp->fbFormat;
    else
	BankFormat = &scrp->formats[0];

    bitsPerPixel = scrp->fbFormat.bitsPerPixel;
    pixmapPad = scrp->fbFormat.scanlinePad;
    if (scrp->depth == 4)
	pixmapPad *= 4;		/* 4 planes */

#define _VIDEOSIZE(w, x, y) \
    ((((((w) * bitsPerPixel) + pixmapPad - 1) / pixmapPad) * pixmapPad * \
     ((y) - 1)) + ((x) * bitsPerPixel))

    /*
     * Determine maxPitch if it wasn't given explicitly.  Note linePitches
     * always takes precedence if is non-NULL.  In that case the minPitch and
     * maxPitch values passed are ignored.
     */
    if (linePitches) {
	minPitch = maxPitch = linePitches[0];
	for (i = 1; linePitches[i] > 0; i++) {
	    if (linePitches[i] > maxPitch)
		maxPitch = linePitches[i];
	    if (linePitches[i] < minPitch)
		minPitch = linePitches[i];
	}
    }

    /* Initial check of virtual size against other constraints */
    scrp->virtualFrom = X_PROBED;
    /*
     * Initialise virtX and virtY if the values are fixed.
     */
    if (virtualY > 0) {
	if (virtualY > maxHeight) {
	    xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		       "Virtual height (%d) is too large for the hardware "
		       "(max %d)\n", virtualY, maxHeight);
	    return -1;
	}

	if (linePitches != NULL) {
	    for (i = 0; linePitches[i] != 0; i++) {
		if ((linePitches[i] >= virtualX) &&
		    (linePitches[i] ==
		     miScanLineWidth(virtualX, virtualY, linePitches[i],
				     apertureSize, BankFormat, pitchInc))) {
		    linePitch = linePitches[i];
		    break;
		}
	    }
	} else {
	    linePitch = miScanLineWidth(virtualX, virtualY, minPitch,
					apertureSize, BankFormat, pitchInc);
	}

	if ((linePitch < minPitch) || (linePitch > maxPitch)) {
	    xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		       "Virtual width (%d) is too large for the hardware "
		       "(max %d)\n", virtualX, maxPitch);
	    return -1;
	}

	if (_VIDEOSIZE(linePitch, virtualX, virtualY) > pixelArea) {
	    xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		      "Virtual size (%dx%d) (pitch %d) exceeds video memory\n",
		      virtualX, virtualY, linePitch);
	    return -1;
	}

	virtX = virtualX;
	virtY = virtualY;
	scrp->virtualFrom = X_CONFIG;
    }

    /*
     * If scrp->modePool hasn't been setup yet, set it up now.  This allows
     * the modes that the driver definitely can't use to be weeded out
     * early.
     */
    if (scrp->modePool == NULL) {
	q = NULL;
	for (p = availModes; p != NULL; p = p->next) {
	    status = xf86InitialCheckModeForDriver(scrp, p, maxPitch, virtualX,
						   virtualY);
	    if (status == MODE_OK) {
		new = (DisplayModePtr)xnfalloc(sizeof(DisplayModeRec));
		*new = *p;
		new->next = NULL;
		if (!q) {
		    scrp->modePool = new;
		} else {
		    q->next = new;
		}
		new->prev = q;
		q = new;
	    } else {
		xf86DrvMsg(scrp->scrnIndex, X_WARNING,
			   "Mode \"%s\" deleted (%s)\n", p->name,
			   xf86ModeStatusToString(status));
	    }
	}
    }

    /*
     * Allocate one entry in scrp->modes for each named mode, if not already
     * done.
     */
    if (scrp->modes == NULL) {
	scrp->modes = (DisplayModePtr)xnfalloc(sizeof(DisplayModeRec));
	scrp->modes->next = NULL;
	scrp->modes->prev = NULL;
    }
    q = scrp->modes;
    for (i = 0; modeNames[i] != NULL; i++) {
	if (firstMode) {
	    firstMode = FALSE;
	} else {
	    if (q->next == NULL) {
		q->next = (DisplayModePtr)xnfalloc(sizeof(DisplayModeRec));
		q->next->next = NULL;
		q->next->name = NULL;
	    } else {
		if (q->next->name)
		    xfree(q->next->name);
	    }
	    q->next->prev = q;
	    q = q->next;
	}
	q->name = (char *)xnfalloc(strlen(modeNames[i]) + 1);
	strcpy(q->name, modeNames[i]);
    }
    q->next = NULL;
    /*
     * XXX Check if there are any remaining previously allocated entries
     * which should be freed?  (but there shouldn't normally be any such
     * entries)
     */

    /* Lookup each mode */
    for (p = scrp->modes; p != NULL; p = p->next) {
	status = xf86LookupMode(scrp, p, clockRanges, strategy);
	if (status == MODE_ERROR) {
	    ErrorF("xf86ValidateModes: "
		   "unexpected result from xf86LookupMode()\n");
	    return -1;
	}
	if (status != MODE_OK) {
	    p->status = status;
	    continue;
	}

	newLinePitch = linePitch;
	newVirtX = virtX;
	newVirtY = virtY;

	/*
	 * Adjust virtual width and height if the mode is too large for the
	 * current values and if they are not fixed.
	 */
	if (virtualX <= 0 && p->HDisplay > newVirtX)
	    newVirtX = p->HDisplay;
	if (virtualY <= 0 && p->VDisplay > newVirtY)
	    newVirtY = p->VDisplay;

	/*
	 * If virtual resolution is to be increased, revalidate it.
	 */
	if ((virtX != newVirtX) || (virtY != newVirtY)) {
	    if (linePitches != NULL) {
		newLinePitch = -1;
		for (i = 0; linePitches[i] != 0; i++) {
		    if ((linePitches[i] >= newVirtX) &&
			(linePitches[i] >= linePitch) &&
			(linePitches[i] ==
			 miScanLineWidth(newVirtX, newVirtY, linePitches[i],
					 apertureSize, BankFormat, pitchInc))) {
			newLinePitch = linePitches[i];
			break;
		    }
		}
	    } else {
		if (linePitch < minPitch)
		    linePitch = minPitch;
		newLinePitch = miScanLineWidth(newVirtX, newVirtY, linePitch,
					       apertureSize, BankFormat,
					       pitchInc);
	    }
	    if ((newLinePitch < minPitch) || (newLinePitch > maxPitch)) {
		p->status = MODE_BAD_WIDTH;
		continue;
	    }

	    /*
	     * Check that the pixel area required by the new virtual height
	     * and line pitch isn't too large.
	     */
	    if (_VIDEOSIZE(newLinePitch, newVirtX, newVirtY) > pixelArea) {
		p->status = MODE_MEM_VIRT;
		continue;
	    }

	    virtX = newVirtX;
	    virtY = newVirtY;
	    linePitch = newLinePitch;
	}

	/* Mode has passed all the tests */
	p->status = MODE_OK;
	numModes++;
    }

#undef _VIDEOSIZE

    /* Update the ScrnInfoRec parameters */
    
    scrp->virtualX = virtX;
    scrp->virtualY = virtY;
    scrp->displayWidth = linePitch;
    
    /* Make the mode list into a circular list by joining up the ends */
    if (numModes > 0) {
	p = scrp->modes;
	while (p->next != NULL)
	    p = p->next;
	/* p is now the last mode on the list */
	p->next = scrp->modes;
	scrp->modes->prev = p;
    }

    return numModes;
}

/*
 * xf86DeleteMode
 *
 * This function removes a mode from a list of modes.
 *
 * There are two types of mode lists:
 *
 *  - doubly linked linear lists, starting and ending in NULL
 *  - doubly linked circular lists
 *
 */
 
void
xf86DeleteMode(DisplayModePtr *modeList, DisplayModePtr mode)
{
    /* Catch the easy/insane cases */
    if (modeList == NULL || *modeList == NULL || mode == NULL)
	return;

    /* If the mode is at the start of the list, move the start of the list */
    if (*modeList == mode)
	*modeList = mode->next;

    /* If mode is the only one on the list, set the list to NULL */
    if ((mode == mode->prev) && (mode->prev == mode->next)) {
	*modeList = NULL;
    } else {
	if (mode->prev != NULL)
	    mode->prev->next = mode->next;
	if (mode->next != NULL)
	    mode->next->prev = mode->prev;
    }

#if 0
    /*
     * Not all names can be freed like this.  XXX Need to consider if things
     * should be changed so that each copy has its own xalloc'd name.
     */
    if (mode->name)
	xfree(mode->name);
#endif
    if (mode)
	xfree(mode);
}

/*
 * xf86PruneMonitorModes
 *
 * Remove the monitor modes which are inconsistent with the monitor's
 * specs.
 */

void
xf86PruneMonitorModes(MonPtr monp)
{
    DisplayModePtr p, n;
    ModeStatus status;
    Bool first = TRUE;
    float refresh;

    p = monp->Modes;
    while (p != NULL) {
	n = p->next;
	status = xf86CheckModeForMonitor(p, monp);
	if (status != MODE_OK) {
	    if (first) {
		first = FALSE;
		xf86MsgVerb(X_INFO, 2, "Bad modes for monitor \"%s\"\n",
			    monp->id);
	    }
	    xf86MsgVerb(X_INFO, 2, "  Mode \"%s\" deleted (%s", p->name, /*)*/
			xf86ModeStatusToString(status));
	    switch (status) {
	    case MODE_HSYNC:
		xf86ErrorFVerb(2, ": %.2f kHz", (double)p->Clock / p->HTotal);
		break;
	    case MODE_VSYNC:
		refresh = p->Clock * 1000.0 / (p->HTotal * p->VTotal);
		if (p->Flags & V_INTERLACE)
		    refresh *= 2.0;
		if (p->Flags & V_DBLSCAN)
		    refresh /= 2.0;
		if (p->VScan > 1)
		    refresh /= (float)(p->VScan);
		xf86ErrorFVerb(2, ": %.2f Hz", refresh);
		break;
	    default:
		break;
	    }
	    xf86ErrorFVerb(2, /*(*/ ")\n");
	    xf86DeleteMode(&(monp->Modes), p);
	}
	p = n;
    }
}

/*
 * xf86PruneDriverModes
 *
 * Remove modes from the driver's mode list which have been marked as
 * invalid.
 */

void
xf86PruneDriverModes(ScrnInfoPtr scrp)
{
    DisplayModePtr first, p, n;

    p = scrp->modes;
    if (p == NULL)
	return;

    do {
	if (!(first = scrp->modes))
	    return;
	n = p->next;
	if (p->status != MODE_OK) {
	    xf86DrvMsg(scrp->scrnIndex, X_WARNING,
		       "Mode \"%s\" deleted (%s)\n", p->name,
		       xf86ModeStatusToString(p->status));
	    xf86DeleteMode(&(scrp->modes), p);
	}
	p = n;
    } while (p != NULL && p != first);
}


/*
 * xf86SetCrtcForModes
 *
 * Goes through the screen's mode list, and initialises the Crtc
 * parameters for each mode.  The initialisation includes adjustments
 * for interlaced and double scan modes.
 */
void
xf86SetCrtcForModes(ScrnInfoPtr scrp, int adjustFlags)
{
    DisplayModePtr p;

    p = scrp->modes;
    if (p == NULL)
	return;

    do {
	p->CrtcHDisplay		= p->HDisplay;
	p->CrtcHSyncStart	= p->HSyncStart;
	p->CrtcHSyncEnd		= p->HSyncEnd;
	p->CrtcHTotal		= p->HTotal;
	p->CrtcHSkew		= p->HSkew;
	p->CrtcVDisplay		= p->VDisplay;
	p->CrtcVSyncStart	= p->VSyncStart;
	p->CrtcVSyncEnd		= p->VSyncEnd;
	p->CrtcVTotal		= p->VTotal;
        if ((p->Flags & V_INTERLACE) && (adjustFlags & INTERLACE_HALVE_V)) {
	    p->CrtcVDisplay	/= 2;
	    p->CrtcVSyncStart	/= 2;
	    p->CrtcVSyncEnd	/= 2;
	    p->CrtcVTotal	/= 2;
	}
	if (p->Flags & V_DBLSCAN) {
	    p->CrtcVDisplay	*= 2;
	    p->CrtcVSyncStart	*= 2;
	    p->CrtcVSyncEnd	*= 2;
	    p->CrtcVTotal	*= 2;
	}
	if (p->VScan > 1) {
	    p->CrtcVDisplay	*= p->VScan;
	    p->CrtcVSyncStart	*= p->VScan;
	    p->CrtcVSyncEnd	*= p->VScan;
	    p->CrtcVTotal	*= p->VScan;
	}
	p->CrtcHAdjusted = FALSE;
	p->CrtcVAdjusted = FALSE;
	p->CrtcVBlankStart = min(p->CrtcVSyncStart, p->CrtcVDisplay);
	p->CrtcVBlankEnd = max(p->CrtcVSyncEnd, p->CrtcVTotal);
	if ((p->CrtcVBlankEnd - p->CrtcVBlankStart) >= 127) {
	    /* 
	     * V Blanking size must be < 127.
	     * Moving blank start forward is safer than moving blank end
	     * back, since monitors clamp just AFTER the sync pulse (or in
	     * the sync pulse), but never before.
	     */
	     p->CrtcVBlankStart = p->CrtcVBlankEnd-127;
	}
	p->CrtcHBlankStart=min(p->CrtcHSyncStart, p->CrtcHDisplay);
	p->CrtcHBlankEnd=max(p->CrtcHSyncEnd, p->CrtcHTotal);
	if ((p->CrtcHBlankEnd - p->CrtcHBlankStart) >= 63*8) {
	    /*
	     * H Blanking size must be < 63*8. Same remark as above.
	     */
	    p->CrtcHBlankStart = p->CrtcHBlankEnd-63*8;
	}

ErrorF("Mode %s: %d (%d) %d %d (%d) %d %d (%d) %d %d (%d) %d\n", p->name,
	p->CrtcHDisplay, p->CrtcHBlankStart, p->CrtcHSyncStart, p->CrtcHSyncEnd,
	p->CrtcHBlankEnd, p->CrtcHTotal,
	p->CrtcVDisplay, p->CrtcVBlankStart, p->CrtcVSyncStart, p->CrtcVSyncEnd,
	p->CrtcVBlankEnd, p->CrtcVTotal);

	p = p->next;
    } while (p != NULL && p != scrp->modes);
}


void
xf86PrintModes(ScrnInfoPtr scrp)
{
    DisplayModePtr p;
    float hsync, refresh;
    char *desc, *desc2;

    if (scrp == NULL)
	return;

    xf86DrvMsg(scrp->scrnIndex, scrp->virtualFrom, "Virtual size is %dx%d "
	       "(pitch %d)\n", scrp->virtualX, scrp->virtualY,
	       scrp->displayWidth);
    
    p = scrp->modes;
    if (p == NULL)
	return;

    do {
	desc = desc2 = "";
	hsync = (float)p->Clock / (float)p->HTotal;
	refresh = hsync * 1000.0 / p->VTotal;
	if (p->Flags & V_INTERLACE) {
	    refresh *= 2.0;
	    desc = " (I)";
	}
	if (p->Flags & V_DBLSCAN) {
	    refresh /= 2.0;
	    desc = " (D)";
	}
	if (p->VScan > 1) {
	    refresh /= p->VScan;
	    desc2 = " (VScan)";
	}
	if (p->Clock == p->SynthClock) {
	    xf86DrvMsg(scrp->scrnIndex, X_CONFIG, "Mode \"%s\": %.1f MHz, "
		   "%.1f kHz, %.1f Hz%s%s\n",
		   p->name, p->Clock / 1000.0,
		   hsync, refresh, desc, desc2);
	} else {
	    xf86DrvMsg(scrp->scrnIndex, X_CONFIG, "Mode \"%s\": %.1f MHz "
		   "(scaled from %.1f MHz), %.1f kHz, %.1f Hz%s%s\n",
		   p->name, p->Clock / 1000.0, p->SynthClock / 1000.0,
		   hsync, refresh, desc, desc2);
	}
	p = p->next;
    } while (p != NULL && p != scrp->modes);
}
