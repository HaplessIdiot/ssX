# $XFree86$
#
# Unspecified mono-spaced font gets courier
#

MATCH 	spacing = 2	EDIT	face = "Courier"
MATCH	spacing = 1	EDIT	face = "Courier"
MATCH	spacing = 0	EDIT	face = "Charter"

DIR XFT_TYPE1_DIR

#
# Check users config file
#
PATH	"?~/.xftconfig"

# Here are the fonts
MATCH face = "utopia" 			EDIT file = "UTRG____.pfa"
MATCH face = "utopia italic" 		EDIT file = "UTI_____.pfa"
MATCH face = "utopia bold" 		EDIT file = "UTB_____.pfa"
MATCH face = "utopia bold italic" 	EDIT file = "UTBI____.pfa"

#
# Two courier fonts; pick which goes first
#
MATCH face = "courier" 			EDIT file = "c0419bt_.pfb"
MATCH face = "courier italic" 		EDIT file = "c0582bt_.pfb"
MATCH face = "courier bold" 		EDIT file = "c0583bt_.pfb"
MATCH face = "courier bold italic" 	EDIT file = "c0611bt_.pfb"

MATCH face = "courier" 			EDIT file = "cour.pfa"
MATCH face = "courier italic" 		EDIT file = "couri.pfa"
MATCH face = "courier bold" 		EDIT file = "courb.pfa"
MATCH face = "courier bold italic" 	EDIT file = "courbi.pfa"

MATCH face = "charter" 			EDIT file = "c0648bt_.pfb"
MATCH face = "charter italic" 		EDIT file = "c0649bt_.pfb"
MATCH face = "charter bold" 		EDIT file = "c0632bt_.pfb"
MATCH face = "charter bold italic" 	EDIT file = "c0633bt_.pfb"

MATCH face = "cursor" 			EDIT file = "cursor.pfa"

MATCH face = "lucidux serif" 		EDIT file = "lcdxrr.pfa"
MATCH face = "lucidux serif oblique" 	EDIT file = "lcdxro.pfa"
MATCH face = "lucidux sans" 		EDIT file = "lcdxsr.pfa"
MATCH face = "lucidux sans oblique" 	EDIT file = "lcdxso.pfa"
MATCH face = "lucidux mono" 		EDIT file = "lcdxmr.pfa"
MATCH face = "lucidux mono oblique" 	EDIT file = "lcdxmo.pfa"
	
#
# Fill in underspecified fonts with default values
#
MATCH
EDIT
	encoding = "iso8859-1"
	size = 768			# 12 pixels
	rotation = 0

