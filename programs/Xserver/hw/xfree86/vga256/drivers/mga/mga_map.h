/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga_map.h,v 3.2 1996/12/09 11:54:19 dawes Exp $ */

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif

#if PSZ == 8
#define MGANAME(subname) CATNAME(Mga8,subname)
#elif PSZ == 16
#define MGANAME(subname) CATNAME(Mga16,subname)
#elif PSZ == 24
#define MGANAME(subname) CATNAME(Mga24,subname)
#elif PSZ == 32
#define MGANAME(subname) CATNAME(Mga32,subname)
#endif

#ifdef __GNUC__
/* 
 * MGANAME_A macro helps in disabling single accel feature
 * for example, to disable screen-copy, you need to have in 
 * /tmp/accelswitch file following lines:
 *    SolidFill    1
 *    ScreenCopy   0
 *    CPUColorExp  1
 * 
 * Parameters in /tmp/accelswitch file:
 * 1 - enable feature
 * 0 - disable feature
 * 2 - use NoopDDA
 */
#define MGANAME_A(subname) \
( \
    ( tmp = MgaAccelSwitch(CATNAME(Str,subname)) ) == 0 ? \
        CATNAME(xf86AccelInfoRec.,subname) : \
        ( tmp == 1 ? MGANAME(subname) : (void (*)())NoopDDA ) \
)
#else
#define MGANAME_A(subname) MGANAME(subname)
#endif

/*
 * Here's aliases for functions which should be used in 
 * /tmp/accelswitch file to disabling/enabling
 */
#define StrSetupForFillRectSolid   "SolidFill"
#define StrSubsequentFillRectSolid "SolidFill"
#define StrSetupForScreenToScreenCopy   "ScreenCopy"
#define StrSubsequentScreenToScreenCopy "ScreenCopy"
#define StrSetupForCPUToScreenColorExpand   "CPUColorExp"
#define StrSubsequentCPUToScreenColorExpand "CPUColorExp"
#define StrSetupForScreenToScreenColorExpand   "ScreenColorExp"
#define StrSubsequentScreenToScreenColorExpand "ScreenColorExp"
#define StrSetupFor8x8PatternColorExpand   "8x8ColorExp"
#define StrSubsequent8x8PatternColorExpand "8x8ColorExp"
#define StrSubsequentTwoPointLine  "Lines"
#define StrSetClippingRectangle    "Lines"
