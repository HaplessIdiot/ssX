/* $Id : $ */

#ifndef _xf86symtabs_h_
#define _xf86symtabs_h_

#ifdef INIT_SYMTABS

SymTabRec xf86MouseTab[] = {
  { MICROSOFT,	"microsoft" },
  { MOUSESYS,	"mousesystems" },
  { MMSERIES,	"mmseries" },
  { LOGITECH,	"logitech" },
  { BUSMOUSE,	"busmouse" },
  { LOGIMAN,	"mouseman" },
  { PS_2,	"ps/2" },
  { MMHITTAB,	"mmhittab" },
  { GLIDEPOINT,	"glidepoint" },
  { IMSERIAL,	"intellimouse" },
  { THINKING,	"thinkingmouse" },
  { IMPS2,	"imps/2" },
  { THINKINGPS2,"thinkingmouseps/2" },
  { MMANPLUSPS2,"mousemanplusps/2" },
  { GLIDEPOINTPS2,"glidepointps/2" },
  { NETPS2,	"netmouseps/2" },
  { NETSCROLLPS2,"netscrollps/2" },
  { SYSMOUSE,	"sysmouse" },
  { AUTOMOUSE,	"auto" },
  { XQUE,	"xqueue" },
  { OSMOUSE,	"osmouse" },
  { -1,		"" },
};

#else

extern SymTabRec xf86MouseTab[];

#endif /* INIT_SYMTABS */

#endif
