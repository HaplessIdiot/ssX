XCOMM $XFree86: xc/lib/Xft/XftConfig.cpp,v 1.6 2001/04/27 14:55:22 tsi Exp $

dir XFT_TYPE1_DIR

XCOMM
XCOMM alias 'fixed' for 'mono'
XCOMM
match any family == "fixed"		edit family =+ "mono";

XCOMM
XCOMM Check users config file
XCOMM
includeif	"~/.xftconfig"

XCOMM
XCOMM Use Luxi fonts for default faces
XCOMM
match any family == "serif"		edit family += "LuxiSerif";
match any family == "sans"		edit family += "LuxiSans";
match any family == "mono"		edit family += "LuxiMono";

XCOMM
XCOMM Alias between XLFD families and font file family name, prefer local
XCOMM fonts
XCOMM
match any family == "charter"		edit family += "bitstream charter";
match any family == "bitstream charter" edit family =+ "charter";

match any family == "Luxi Serif"	edit family += "LuxiSerif";
match any family == "LuxiSerif"	edit family =+ "Luxi Serif";

match any family == "Luxi Sans"	edit family += "LuxiSans";
match any family == "LuxiSans"	edit family =+ "Luxi Sans";

match any family == "Luxi Mono"	edit family += "LuxiMono";
match any family == "LuxiMono"	edit family =+ "Luxi Mono";
