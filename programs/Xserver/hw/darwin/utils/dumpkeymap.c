// $XFree86$
//
//=============================================================================
//
// Copyright (C) 1999,2000 by Eric Sunshine <sunshine@sunshineco.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//   3. The name of the author may not be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
// NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//-----------------------------------------------------------------------------
// dumpkeymap.c
//
//	Prints a textual representation of an Apple/NeXT .keymapping file, or
//	the currently active key map in use by the WindowServer and the AppKit
//	on the local machine.
//
// KEY MAPPING DESCRIPTION
//
//	Types and Data
//	--------------
//	The following type definitions are employed throughout this
//	discussion:
//
//	typedef unsigned char  byte;
//	typedef unsigned short word;
//	typedef unsigned long  dword;
//
//	Additionally, the type definition `number' is used generically to
//	indicate a numeric value.  The actual size of the `number' type may be
//	one or two bytes depending upon how the data is stored in the key map.
//	Although most key maps use byte-sized numeric values, word-sized
//	values are also allowed.
//
//	Multi-byte values in a key mapping file are stored in big-endian byte
//	order.
//
//	Key Mapping File and Device Mapping
//	-----------------------------------
//	A key mapping file begins with a magic-number and continues with a
//	variable number of device-specific key mappings.
//
// 	struct KeyMappingFile {
// 	    char magic_number[4];	// "KYM1"
// 	    DeviceMapping maps[...]	// Variable number of maps
// 	};
//
// 	struct DeviceMapping {
// 	    dword interface;	// NX_EVS_DEVICE_INTERFACE_ACE, etc.
// 	    dword handler_id;	// Interface subtype (0=101, 1=102 key, etc.)
// 	    dword map_size;	// Byte count following this address
// 	    KeyMapping map;
// 	};
//
//	Together, `interface' and `handler_id' identify the exact keyboard
//	hardware to which this mapping applies.  The `interface' value
//	represents a family of keyboard device types (such as Intel, ADB,
//	NeXT, Sun Type5, etc.) and `handler_id' represents a specific keyboard
//	layout within that family.  On MacOS/X and Darwin, `interface'
//	constants can be found in IOHIDTypes.h, whereas on MacOS/X Server,
//	OpenStep, and NextStep, they can be found in ev_types.h.  Programs
//	which display a visual representation of a keyboard layout, match
//	`interface' and `handler_id' from the .keymapping file against the
//	`interface' and `handler_id' values found in each .keyboard file.
//
//	Key Mapping
//	-----------
//	A key mapping completely defines the relationship of all scan codes
//	with their associated functionality.  A KeyMapping structure is
//	embedded within the DeviceMapping structure in a KeyMappingFile.  The
//	currently active key mapping in use by the WindowServer and AppKit is
//	also represented by a KeyMapping structure, and can be referred to
//	directly by calling NXGetKeyMapping() and accessing the `mapping' data
//	member of the returned NXKeyMapping structure.
//
// 	struct KeyMapping {
//	    word number_size;			// 0=1 byte, non-zero=2 bytes
//	    number num_modifier_groups;		// Modifier groups
//	    ModifierGroup modifier_groups[...];
//	    number num_scan_codes;		// Scan groups
//	    ScanGroup scan_table[...];
//	    number num_sequence_lists;		// Sequence lists
//	    Sequence sequence_lists[...];
//	    number num_special_keys;		// Special keys
//	    SpecialKey special_key[...];
// 	};
//
//	The `number_size' flag determines the size, in bytes, of all remaining
//	numeric values (denoted by the type definition `number') within the
//	key mapping.  If its value is zero, then numbers are represented by a
//	single byte.  If it is non-zero, then numbers are represented by a
//	word (two bytes).
//
//	Modifier Group
//	--------------
//	A modifier group defines all scan codes which map to a particular type
//	of modifier, such as "shift", "control", etc.
//
// 	enum Modifier {
// 	    ALPHALOCK = 0,
// 	    SHIFT,
// 	    CONTROL,
// 	    ALTERNATE,
// 	    COMMAND,
// 	    KEYPAD,
// 	    HELP
// 	};
//
// 	struct ModifierGroup {
// 	    number modifier;		// A Modifier constant
// 	    number num_scan_codes;
// 	    number scan_codes[...];	// Variable number of scan codes
// 	};
//
//	The scan_codes[] array contains a list of all scan codes which map to
//	the specified modifier.  The "shift", "command", and "alternate"
//	modifiers are frequently mapped to two different scan codes, apiece,
//	since these modifiers often appear on both the left and right sides of
//	the keyboard.
//
//	Scan Group
//	----------
//	There is one ScanGroup for each scan code generated by the given
//	keyboard.  This number is given by KeyMapping::num_scan_codes.  The
//	first scan group represents hardware scan code 0, the second
//	represents scan code 1, etc.
//
//	enum ModifierMask {
// 	    ALPHALOCK_MASK	 = 1 << 0,
// 	    SHIFT_MASK		 = 1 << 1,
// 	    CONTROL_MASK	 = 1 << 2,
// 	    ALTERNATE_MASK	 = 1 << 3,
// 	    CARRIAGE_RETURN_MASK = 1 << 4
// 	};
// 	#define NOT_BOUND 0xff
//
// 	struct ScanGroup {
// 	    number mask;
// 	    Character characters[...];
// 	};
//
//	For each scan code, `mask' defines which modifier combinations
//	generate characters.  If `mask' is NOT_BOUND (0xff) then then this
//	scan code does not generate any characters ever, and its characters[]
//	array is zero length.  Otherwise, the characters[] array contains one
//	Character record for each modifier combination.
//
//	The number of records in characters[] is determined by computing
//	(1 << bits_set_in_mask).  In other words, if mask is zero, then zero
//	bits are set, so characters[] contains only one record.  If `mask' is
//	(SHIFT_MASK | CONTROL_MASK), then two bits are set, so characters[]
//	contains four records.
//
//	The first record always represents the character which is generated by
//	that key when no modifiers are active.  The remaining records
//	represent characters generated by the various modifier combinations.
//	Using the example with the "shift" and "control" masks set, record two
//	would represent the character with the shift modifier active; record
//	three, the control modifier active; and record four, both the shift
//	and control modifiers active.
//
//	As a special case, ALPHALOCK_MASK implies SHIFT_MASK, though only
//	ALPHALOCK_MASK appears in `mask'.  In this case the same character is
//	generated for both the shift and alpha-lock modifiers, but only needs
//	to appear once in the characters[] array.
//
//	Character
//	---------
//	Each Character record indicates the character generated when this key
//	is pressed, as well as the character set which contains the character.
//	Well known character Sets are "ASCII" and "Symbol".  The character set
//	can also be one of the meta values FUNCTION_KEY or KEY_SEQUENCE.  If
//	it is FUNCTION_KEY then `char_code' represents a generally well-known
//	function key such as those enumerated by FunctionKey.  If the
//	character set is KEY_SEQUENCE then `char_code' represents is a
//	zero-base index into KeyMapping::sequence_lists[].
//
// 	enum CharacterSet {
// 	    ASCII	 = 0x00,
// 	    SYMBOL	 = 0x01,
// 	    ...
// 	    FUNCTION_KEY = 0xfe,
// 	    KEY_SEQUENCE = 0xff
// 	};
//
// 	struct Character {
// 	    number set;		// CharacterSet of generated character
// 	    number char_code;	// Actual character generated
// 	};
//
// 	enum FunctionKey {
// 	    F1 = 0x20, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
// 	    INSERT, DELETE, HOME, END, PAGE_UP, PAGE_DOWN, PRINT_SCREEN,
// 	    SCROLL_LOCK, PAUSE, SYS_REQUEST, BREAK, RESET, STOP, MENU, USER,
// 	    SYSTEM, PRINT, CLEAR_LINE, CLEAR_DISPLAY, INSERT_LINE,
// 	    DELETE_LINE, INSERT_CHAR, DELETE_CHAR, PREV, NEXT, SELECT
// 	};
//
//	Sequence
//	--------
//	When Character::set contains the meta value KEY_SEQUENCE, the scan
//	code is bound to a sequence of keys rather than a single character.  A
//	sequence is a series of modifiers and characters which are
//	automatically generated when the associated key is depressed.  Each
//	generated Character is represented as previously described, with the
//	exception that MODIFIER_KEY may appear in place of KEY_SEQUENCE.  When
//	the value of Character::set is MODIFIER_KEY then Character::char_code
//	represents a modifier key rather than an actual character.  If the
//	modifier represented by `char_code' is non-zero, then it indicates
//	that the associated modifier key has been depressed.  In this case,
//	the value is one of the constants enumerated by Modifier (SHIFT,
//	CONTROL, ALTERNATE, etc.).  If the value is zero then it means that
//	the modifier keys have been released.
//
// 	#define MODIFIER_KEY 0xff
//
// 	struct Sequence {
// 	    number num_chars;
// 	    Character characters[...];
// 	};
//
//	Special Key
//	-----------
//	A special key is one which is scanned directly by the Mach kernel
//	rather than by the WindowServer.  In general, events are not generated
//	for special keys.
//
// 	enum SpecialKeyType {
//	    VOLUME_UP = 0,
//	    VOLUME_DOWN,
//	    BRIGHTNESS_UP,
//	    BRIGHTNESS_DOWN,
//	    ALPHA_LOCK,
//	    HELP,
//	    POWER,
//	    SECONDARY_ARROW_UP,
//	    SECONDARY_ARROW_DOWN
// 	};
//
// 	struct SpecialKey {
// 	    number type;	// A SpecialKeyType constant
// 	    number scan_code;	// Actual scan code
// 	};
//
// COMPILATION INSTRUCTIONS
//
//	MacOS/X, Darwin
//	cc -Wall -o dumpkeymap dumpkeymap.c -framework IOKit
//
//	MacOS/X Server, OpenStep, NextStep
//	cc -Wall -o dumpkeymap dumpkeymap.c
//
// USAGE INSTRUCTIONS
//
//	Usage: dumpkeymap [path-to-keymap ...]
//
//	When provided with no arguments, this program dumps the currently
//	active key map.  Otherwise, it prints out the contents of each
//	.keymapping files mentioned as an argument on the command line.
//
// CONCLUSION
//
//	This program and its accompanying documentation were written by Eric
//	Sunshine and are copyright (C)1999,2000 by Eric Sunshine
//	<sunshine@sunshineco.com>.  It is based on information gathered on
//	September 3, 1997 by Eric Sunshine and Paul S. McCarthy
//	<zarnuk@zarnuk.com> while reverse engineering the NeXT .keymapping
//	file format.
//
// HISTORY
//
//	2000/11/13 Eric Sunshine <sunshine@sunshineco.com>
//	    Converted from C++ to plain-C.
//	    Now parses and takes into account the "number-size" flag stored
//		with each key map.  This flag indicates the size, in bytes, of
//		all remaining numeric values in the mapping.  Updated all code
//		to respect the this flag.  (Previously, the purpose of this
//		field was unknown, and it was thus denoted as
//		`KeyMapping::fill[2]'.)
//	    Updated all documentation; especially the "KEY MAPPING
//		DESCRIPTION" section.  Added discussion of the "number-size"
//		flag and revamped all structure definitions to use the generic
//		data type `number' instead of `uchar' or 'byte'.  Clarified
//		several sections of the documentation and added missing
//		discussions about type definitions and the relationship of
//		`interface' and `handler_id' to .keymapping and .keyboard
//		files.
//	    Updated compilation instructions to include directions for all
//		platforms on which this program might be built.
//	    Now published under the formal BSD license rather than a
//		home-grown license.
//
//	1999/09/08 Eric Sunshine <sunshine@sunshineco.com>
//	    Created.
//-----------------------------------------------------------------------------
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <drivers/event_status_driver.h>
#include <sys/stat.h>

#define PROG_NAME "dumpkeymap"
#define PROG_VERSION 2
#define AUTHOR_NAME "Eric Sunshine"
#define AUTHOR_EMAIL "sunshine@sunshineco.com"
#define AUTHOR_INFO AUTHOR_NAME " <" AUTHOR_EMAIL ">"
#define COPYRIGHT "Copyright (C) 1999,2000 by " AUTHOR_INFO

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int natural;
typedef unsigned long dword;
typedef dword number;

#define ASCII_SET 0x00
#define BIND_FUNCTION 0xfe
#define BIND_SPECIAL 0xff

//-----------------------------------------------------------------------------
// Translation Tables
//-----------------------------------------------------------------------------
static char const* const SPECIAL_CODE[] =
    {
    "sound-up",
    "sound-down",
    "brightness-up",
    "brightness-down",
    "alpha-lock",
    "help",
    "power",
    "secondary-up-arrow",
    "secondary-down-arrow"
    };
#define N_SPECIAL_CODE (sizeof(SPECIAL_CODE) / sizeof(SPECIAL_CODE[0]))

static char const* const MODIFIER_CODE[] =
    {
    "alpha-lock",
    "shift",
    "control",
    "alternate",
    "command",
    "keypad",
    "help"
    };
#define N_MODIFIER_CODE (sizeof(MODIFIER_CODE) / sizeof(MODIFIER_CODE[0]))

static char const* const MODIFIER_MASK[] =
    {
    "-----",	// R = carriage-return
    "----L",	// A = alternate
    "---S-",	// C = control
    "---SL",	// S = shift
    "--C--",	// L = alpha-lock
    "--C-L",
    "--CS-",
    "--CSL",
    "-A---",
    "-A--L",
    "-A-S-",
    "-A-SL",
    "-AC--",
    "-AC-L",
    "-ACS-",
    "-ACSL",
    "R----",
    "R---L",
    "R--S-",
    "R--SL",
    "R-C--",
    "R-C-L",
    "R-CS-",
    "R-CSL",
    "RA---",
    "RA--L",
    "RA-S-",
    "RA-SL",
    "RAC--",
    "RAC-L",
    "RACS-",
    "RACSL",
    };
#define N_MODIFIER_MASK (sizeof(MODIFIER_MASK) / sizeof(MODIFIER_MASK[0]))

#define FUNCTION_KEY_FIRST 0x20
static char const* const FUNCTION_KEY[] =
    {
    "F1",			// 0x20
    "F2",			// 0x21
    "F3",			// 0x22
    "F4",			// 0x23
    "F5",			// 0x24
    "F6",			// 0x25
    "F7",			// 0x26
    "F8",			// 0x27
    "F9",			// 0x28
    "F10",			// 0x29
    "F11",			// 0x2a
    "F12",			// 0x2b
    "insert",			// 0x2c
    "delete",			// 0x2d
    "home",			// 0x2e
    "end",			// 0x2f
    "page up",			// 0x30
    "page down",		// 0x31
    "print screen",		// 0x32
    "scroll lock",		// 0x33
    "pause",			// 0x34
    "sys-request",		// 0x35
    "break",			// 0x36
    "reset (HIL)",		// 0x37
    "stop (HIL)",		// 0x38
    "menu (HIL)",		// 0x39
    "user (HIL)",		// 0x3a
    "system (HIL)",		// 0x3b
    "print (HIL)",		// 0x3c
    "clear line (HIL)",		// 0x3d
    "clear display (HIL)",	// 0x3e
    "insert line (HIL)",	// 0x3f
    "delete line (HIL)",	// 0x40
    "insert char (HIL)",	// 0x41
    "delete char (HIL)",	// 0x42
    "prev (HIL)",		// 0x43
    "next (HIL)",		// 0x44
    "select (HIL)",		// 0x45
    };
#define N_FUNCTION_KEY (sizeof(FUNCTION_KEY) / sizeof(FUNCTION_KEY[0]))


//-----------------------------------------------------------------------------
// Data Stream Object
//	Can be configured to treat embedded "numbers" as being composed of
//	either 1, 2, or 4 bytes, apiece.
//-----------------------------------------------------------------------------
typedef struct _DataStream
    {
    byte const* data;
    byte const* data_end;
    natural number_size; // Size in bytes of a "number" in the stream.
    } DataStream;

static DataStream* new_data_stream( byte const* data, int size )
    {
    DataStream* s = (DataStream*)malloc( sizeof(DataStream) );
    s->data = data;
    s->data_end = data + size;
    s->number_size = 1; // Default to byte-sized numbers.
    return s;
    }

static void destroy_data_stream( DataStream* s )
    {
    free(s);
    }

static int end_of_stream( DataStream* s )
    {
    return (s->data >= s->data_end);
    }

static void expect_nbytes( DataStream* s, int nbytes )
    {
    if (s->data + nbytes > s->data_end)
	{
	fprintf( stderr, "Insufficient data in keymapping data stream.\n" );
	exit(-1);
	}
    }

static byte get_byte( DataStream* s )
    {
    expect_nbytes( s, 1 );
    return *s->data++;
    }

static word get_word( DataStream* s )
    {
    word hi, lo;
    expect_nbytes( s, 2 );
    hi = *s->data++;
    lo = *s->data++;
    return ((hi << 8) | lo);
    }

static dword get_dword( DataStream* s )
    {
    dword b1, b2, b3, b4;
    expect_nbytes( s, 4 );
    b4 = *s->data++;
    b3 = *s->data++;
    b2 = *s->data++;
    b1 = *s->data++;
    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
    }

static number get_number( DataStream* s )
    {
    switch (s->number_size)
	{
	case 4:  return get_dword(s);
	case 2:  return get_word(s);
	default: return get_byte(s);
	}
    }


//-----------------------------------------------------------------------------
// Translation Utility Functions
//-----------------------------------------------------------------------------
static char const* special_code_desc( number n )
    {
    if (n < N_SPECIAL_CODE)
	return SPECIAL_CODE[n];
    else
	return "invalid";
    }

static char const* modifier_code_desc( number n )
    {
    if (n < N_MODIFIER_CODE)
	return MODIFIER_CODE[n];
    else
	return "invalid";
    }

static char const* modifier_mask_desc( number n )
    {
    if (n < N_MODIFIER_MASK)
	return MODIFIER_MASK[n];
    else
	return "?????";
    }

static char const* function_key_desc( number n )
    {
    if (n >= FUNCTION_KEY_FIRST && n < N_FUNCTION_KEY + FUNCTION_KEY_FIRST)
	return FUNCTION_KEY[ n - FUNCTION_KEY_FIRST ];
    else
	return "unknown";
    }

static number bits_set( number mask )
    {
    number n = 0;
    for ( ; mask != 0; mask >>= 1)
	if ((mask & 0x01) != 0)
	    n++;
    return n;
    }


//-----------------------------------------------------------------------------
// Unparse a list of Modifier records.
//-----------------------------------------------------------------------------
static void unparse_modifiers( DataStream* s )
    {
    number nmod = get_number(s); // Modifier count
    printf( "\nMODIFIERS [%lu]\n", nmod );
    while (nmod-- > 0)
	{
	number nscan;
	number const code = get_number(s);
	printf( "%s:", modifier_code_desc(code) );
	nscan = get_number(s);
	while (nscan-- > 0)
	    printf( " 0x%02x", (natural)get_number(s) );
	putchar( '\n' );
	}
    }


//-----------------------------------------------------------------------------
// Unparse a list of Character records.
//-----------------------------------------------------------------------------
typedef void (*UnparseSpecialFunc)( number code );

static void unparse_char_codes(
    DataStream* s, number ncodes, UnparseSpecialFunc unparse_special )
    {
    if (ncodes != 0)
	{
	while (ncodes-- > 0)
	    {
	    number const char_set = get_number(s);
	    number const code = get_number(s);
	    putchar(' ');
	    switch (char_set)
		{
		case ASCII_SET:
		    {
		    int const c = (int)code;
		    if (isprint(c))
			printf( "\"%c\"", c );
		    else if (code < ' ')
			printf( "\"^%c\"", c + '@' );
		    else
			printf( "%02x", c );
		    break;
		    }
		case BIND_FUNCTION:
		    printf( "[%s]", function_key_desc(code) );
		    break;
		case BIND_SPECIAL:
		    unparse_special( code );
		    break;
		default:
		    printf( "%02x/%02x", (natural)char_set, (natural)code );
		    break;
		}
	    }
	}
    }


//-----------------------------------------------------------------------------
// Unparse a list of scan code bindings.
//-----------------------------------------------------------------------------
static void unparse_key_special( number code )
    {
    printf( "{seq#%lu}", code );
    }

static void unparse_keys( DataStream* s )
    {
    number const NOT_BOUND = 0xff;
    number const nkeys = get_number(s);
    number scan;
    printf( "\nKEYS [%lu]\n", nkeys );
    for (scan = 0; scan < nkeys; scan++)
	{
	number const mask = get_number(s);
	printf( "scan 0x%02x: ", (natural)scan );
	if (mask == NOT_BOUND)
	    printf( "not-bound\n" );
	else
	    {
	    number const bits = bits_set( mask );
	    number const codes = 1 << bits;
	    printf( "%s ", modifier_mask_desc(mask) );
	    unparse_char_codes( s, codes, unparse_key_special );
	    putchar( '\n' );
	    }
	}
    }


//-----------------------------------------------------------------------------
// Unparse a list of key sequences.
//-----------------------------------------------------------------------------
static void unparse_sequence_special( number code )
    {
    printf( "{%s}", (code == 0 ? "unmodify" : modifier_code_desc(code)) );
    }

static void unparse_sequences( DataStream* s )
    {
    number const nseqs = get_number(s);
    number seq;
    printf( "\nSEQUENCES [%lu]\n", nseqs );
    for (seq = 0; seq < nseqs; seq++)
	{
	number const nchars = get_number(s);
	printf( "sequence %lu: ", seq );
	unparse_char_codes( s, nchars, unparse_sequence_special );
	putchar( '\n' );
	}
    }


//-----------------------------------------------------------------------------
// Unparse a list of special keys.
//-----------------------------------------------------------------------------
static void unparse_specials( DataStream* s )
    {
    number nspecials = get_number(s);
    printf( "\nSPECIALS [%lu]\n", nspecials );
    while (nspecials-- > 0)
	{
	number const special = get_number(s);
	number const scan = get_number(s);
	printf( "%s: 0x%02x\n", special_code_desc(special), (natural)scan );
	}
    }


//-----------------------------------------------------------------------------
// Unparse the number-size flag.
//-----------------------------------------------------------------------------
static void unparse_numeric_size( DataStream* s )
    {
    word const numbers_are_shorts = get_word(s);
    s->number_size = numbers_are_shorts ? 2 : 1;
    }


//-----------------------------------------------------------------------------
// Unparse an entire key map.
//-----------------------------------------------------------------------------
static void unparse_keymap_data( DataStream* s )
    {
    unparse_numeric_size(s);
    unparse_modifiers(s);
    unparse_keys(s);
    unparse_sequences(s);
    unparse_specials(s);
    }


//-----------------------------------------------------------------------------
// Unparse the active key map.
//-----------------------------------------------------------------------------
static int unparse_active_keymap( void )
    {
    int rc = 1;
    NXEventHandle const h = NXOpenEventStatus();
    if (h == 0)
	fprintf( stderr, "Unable to open event status driver.\n" );
    else
	{
	NXKeyMapping km;
	km.size = NXKeyMappingLength(h);
	if (km.size <= 0)
	    fprintf( stderr, "Bad key mapping length (%d).\n", km.size );
	else
	    {
	    km.mapping = (char*)malloc( km.size );
	    if (NXGetKeyMapping( h, &km ) == 0)
		fprintf( stderr, "Unable to get current key mapping.\n" );
	    else
		{
		DataStream* stream =
		    new_data_stream( (byte const*)km.mapping, km.size );
		unparse_keymap_data( stream );
		destroy_data_stream( stream );
		rc = 0;
		}
	    free( km.mapping );
	    }
	NXCloseEventStatus(h);
	}
    return rc;
    }


//-----------------------------------------------------------------------------
// Unparse one key map from a keymapping file.
//-----------------------------------------------------------------------------
static void unparse_keymap( DataStream* s )
    {
    dword const interface = get_dword(s);
    dword const handler_id = get_dword(s);
    dword const map_size = get_dword(s);
    printf( "interface=0x%02lx handler_id=0x%02lx map_size=%lu bytes\n",
	interface, handler_id, map_size );
    unparse_keymap_data(s);
    }


//-----------------------------------------------------------------------------
// Check the magic number of a keymapping file.
//-----------------------------------------------------------------------------
static int check_magic_number( DataStream* s )
    {
    return (get_byte(s) == 'K' &&
	    get_byte(s) == 'Y' &&
	    get_byte(s) == 'M' &&
	    get_byte(s) == '1');
    }


//-----------------------------------------------------------------------------
// Unparse all key maps within a keymapping file.
//-----------------------------------------------------------------------------
static int unparse_keymaps( DataStream* s )
    {
    int rc = 0;
    if (check_magic_number(s))
	{
	int n = 0;
	while (!end_of_stream(s))
	    {
	    printf( "\nKEYMAP #%d: ", n++ );
	    unparse_keymap(s);
	    }
	}
    else
	{
	fprintf( stderr, "Bad magic number.\n" );
	rc = 1;
	}
    return rc;
    }


//-----------------------------------------------------------------------------
// Unparse a keymapping file.
//-----------------------------------------------------------------------------
static int unparse_keymap_file( char const* const path )
    {
    int rc = 1;
    FILE* file;
    printf( "\nKEYMAP FILE: %s\n", path );
    file = fopen( path, "rb" );
    if (file == 0)
	perror( "Unable to open keymap" );
    else
	{
	struct stat st;
	if (fstat( fileno(file), &st ) != 0)
	    perror( "Unable to determine file size" );
	else
	    {
	    byte* buffer = (byte*)malloc( st.st_size );
	    if (fread( buffer, st.st_size, 1, file ) != 1)
		perror( "Unable to read keymap" );
	    else
		{
		DataStream* stream = new_data_stream(buffer, (int)st.st_size);
		fclose( file ); file = 0;
		rc = unparse_keymaps( stream );
		destroy_data_stream( stream );
		}
	    free( buffer );
	    }
	if (file != 0)
	    fclose( file );
	}
    return rc;
    }


//-----------------------------------------------------------------------------
// Print an informational banner.
//-----------------------------------------------------------------------------
static void print_banner( void )
    {
    printf( "\n" PROG_NAME " v%d by " AUTHOR_INFO "\n" COPYRIGHT "\n",
	PROG_VERSION );
    }


//-----------------------------------------------------------------------------
// Master dispatcher.
//-----------------------------------------------------------------------------
int main( int const argc, char const* const argv[] )
    {
    int rc = 0;
    print_banner();
    if (argc == 1)	// No arguments, unparse keymap currently in use.
	rc = unparse_active_keymap();
    else		// Unparse keymaps specified on command line.
	{
	int i;
	for (i = 1; i < argc; i++)
	    rc |= unparse_keymap_file( argv[i] );
	}
    return rc;
    }

