/**************************************************************
 *
 * Startup code for the IOKit Darwin X Server
 *
 **************************************************************/
/* $XFree86: $ */

#include "mi.h"
#include "mipointer.h"
#include "scrnintstr.h"

/*
 * DarwinHandleGUI
 *  This function is called first from main().
 *  It does nothing for the IOKit X server.
 */
void DarwinHandleGUI(
    int         argc,
    char        *argv[],
    char        *envp[] )
{
}

// No Quartz support. All Quartz functions are no-ops.

BOOL QuartzAddScreen(ScreenPtr pScreen) {
    FatalError("QuartzAddScreen called without Quartz support.\n");
}

void QuartzOsVendorInit(void) {
    FatalError("QuartzOsVendorInit called without Quartz support.\n");
}

void QuartzGiveUp(void) {
    FatalError("QuartzGiveUp called without Quartz support.\n");
}

void QuartzHide(void) {
    FatalError("QuartzHide called without Quartz support.\n");
}

void QuartzShow(void) {
    FatalError("QuartzShow called without Quartz support.\n");
}

void QuartzReadPasteboard(void) {
    FatalError("QuartzReadPasteboard called without Quartz support.\n");
}

void QuartzWritePasteboard(void) {
    FatalError("QuartzWritePasteboard called without Quartz support.\n");
}
