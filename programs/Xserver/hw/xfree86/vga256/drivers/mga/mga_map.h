/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga_map.h,v 3.0 1996/11/18 13:18:08 dawes Exp $ */

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

#define MGANAME_A(subname) \
( \
    ( tmp = MgaAccelSwitch(CATNAME(Str,subname)) ) == 0 ? \
        CATNAME(xf86AccelInfoRec.,subname) : \
        ( tmp == 1 ? MGANAME(subname) : (void (*)())NoopDDA ) \
)

#define StrSetupForFillRectSolid   "SolidFill"
#define StrSubsequentFillRectSolid "SolidFill"
#define StrSetupForScreenToScreenCopy   "ScreenCopy"
#define StrSubsequentScreenToScreenCopy "ScreenCopy"
#define StrSetupForCPUToScreenColorExpand   "CPUColorExp"
#define StrSubsequentCPUToScreenColorExpand "CPUColorExp"
#define StrSetupForScreenToScreenColorExpand   "ScreenColorExp"
#define StrSubsequentScreenToScreenColorExpand "ScreenColorExp"
