//=============================================================================
//
// Keycode mapping support for Xmaster
//
// By Torrey T. Lyons
//
// The code to parse the Darwin keymap is copied from darwinKeyboard.c,
// which was derived from dumpkeymap.c by Eric Sunshine.
// dumpkeymap.c includes the following license:
//
//-----------------------------------------------------------------------------
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

// The Carbon Event Model only allows us access to virtual keycodes,
// not the real keycodes, which the X server uses. The two are the
// same except for arrow and modifier keys. These routines read the
// IOKit keyboard mapping to find the real keycodes to use in these
// cases.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <drivers/event_status_driver.h>
#include <IOKit/hidsystem/ev_keymap.h>

// The arrow keys are members of the symbol character set.
// Their character codes are defined in order as:
//   Left, Up, Right, Down
// These need to be mapped to Carbon keycodes, which go:
//   Left, Right, Down, Up
#define MIN_SYMBOL		0xAC
static int const symbol_to_carbon[] = {
    0,        3,          1,      2
  };
int const NUM_SYMBOL = 4;

static unsigned char modifierKeycodes[NX_NUMMODIFIERS][2];
static unsigned char arrowKeycodes[4];

//-----------------------------------------------------------------------------
// Data Stream Object
//	Can be configured to treat embedded "numbers" as being composed of
//	either 1, 2, or 4 bytes, apiece.
//-----------------------------------------------------------------------------
typedef struct _DataStream
{
    unsigned char const *data;
    unsigned char const *data_end;
    short number_size;  // Size in bytes of a "number" in the stream.
} DataStream;

static DataStream* new_data_stream( unsigned char const* data, int size )
{
    DataStream* s = (DataStream*)malloc( sizeof(DataStream) );
    assert( s );
    s->data = data;
    s->data_end = data + size;
    s->number_size = 1; // Default to byte-sized numbers.
    return s;
}

static void destroy_data_stream( DataStream* s )
{
    free(s);
}

static unsigned char get_byte( DataStream* s )
{
    assert(s->data + 1 <= s->data_end);
    return *s->data++;
}

static short get_word( DataStream* s )
{
    short hi, lo;
    assert(s->data + 2 <= s->data_end);
    hi = *s->data++;
    lo = *s->data++;
    return ((hi << 8) | lo);
}

static int get_dword( DataStream* s )
{
    int b1, b2, b3, b4;
    assert(s->data + 4 <= s->data_end);
    b4 = *s->data++;
    b3 = *s->data++;
    b2 = *s->data++;
    b1 = *s->data++;
    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

static int get_number( DataStream* s )
{
    switch (s->number_size) {
	case 4:  return get_dword(s);
	case 2:  return get_word(s);
	default: return get_byte(s);
    }
}

//-----------------------------------------------------------------------------
// Utility functions to help parse Darwin keymap
//-----------------------------------------------------------------------------

/*
 * bits_set
 * Calculate number of bits set in the modifier mask.
 */
static short bits_set( short mask )
{
    short n = 0;

    for ( ; mask != 0; mask >>= 1)
	if ((mask & 0x01) != 0)
	    n++;
    return n;
}

/*
 * parse_next_char_code
 * Read the next character code from the Darwin keymapping
 * and record it if it is an arrow character.
 */
static void parse_next_char_code(
    int		keyCode,
    DataStream  *s )
{
    const short charSet = get_number(s);
    const short charCode = get_number(s);

    if (charSet == 0x01) {       // symbol character
        if (charCode >= MIN_SYMBOL &&
            charCode <= MIN_SYMBOL + NUM_SYMBOL)
            arrowKeycodes[symbol_to_carbon[charCode - MIN_SYMBOL]] = keyCode; 
    }
}

/*
 * InitKeyMap
 *      Get the Darwin keyboard map and find the arrow and
 *	modifier keycodes.
 */
void InitKeyMap()
{
    int                 i;
    short		numMods, numKeys;
    NXEventHandle	hidParam;
    NXKeyMapping        keyMap;
    DataStream		*keyMapStream;

    memset( arrowKeycodes, 0, sizeof( arrowKeycodes ) );
    for (i = 0; i < NX_NUMMODIFIERS; i++) {
        modifierKeycodes[i][0] = modifierKeycodes[i][1] = 0;
    }

    // Open a shared connection to the HID System.
    // Note that the Event Status Driver is really just a wrapper
    // for a kIOHIDParamConnectType connection.
    assert( hidParam = NXOpenEventStatus() );

    // get the Darwin keyboard map
    keyMap.size = NXKeyMappingLength( hidParam );
    assert( keyMap.mapping = (char*) malloc( keyMap.size ));
    assert( NXGetKeyMapping( hidParam, &keyMap ));
    keyMapStream = new_data_stream( (unsigned char const*)keyMap.mapping,
                                    keyMap.size );

    // check the type of map
    if (get_word(keyMapStream)) {
        keyMapStream->number_size = 2;
        printf("Current 16-bit keymapping may not be interpreted correctly.\n");
    }

    // Compute the modifier map
    numMods = get_number(keyMapStream);
    while (numMods-- > 0) {
        int	    	left = 1;                   // first keycode is left
        short const     charCode = get_number(keyMapStream);
        short           numKeyCodes = get_number(keyMapStream);
        while (numKeyCodes-- > 0) {
            const short keyCode = get_number(keyMapStream);
            modifierKeycodes[charCode][1-left] = keyCode;
            left = 0;
        }
    }

    // Look through Darwin keyboard map for the arrow keys.
    numKeys = get_number(keyMapStream);
    for (i = 0; i < numKeys; i++) {
        short const     charGenMask = get_number(keyMapStream);
        if (charGenMask != 0xFF) {              // is key bound?
            short       numKeyCodes = 1 << bits_set(charGenMask);

            // If alphalock and shift modifiers produce different codes,
            // we only need the shift case since X handles alphalock.
            if (charGenMask & 0x01 && charGenMask & 0x02) {
                // record unshifted case
                parse_next_char_code( i, keyMapStream );
                // skip alphalock case
                get_number(keyMapStream); get_number(keyMapStream);
                // record shifted case
                parse_next_char_code( i, keyMapStream );
                numKeyCodes -= 3;
                // skip the rest
                while (numKeyCodes-- > 0) {
                    get_number(keyMapStream); get_number(keyMapStream);
                }

            // If alphalock and shift modifiers produce same code, use it.
            } else if (charGenMask & 0x03) {
                // record unshifted case
                parse_next_char_code( i, keyMapStream );
                // record shifted case
                parse_next_char_code( i, keyMapStream );
                numKeyCodes -= 2;
                // skip the rest
                while (numKeyCodes-- > 0) {
                    get_number(keyMapStream); get_number(keyMapStream);
                }

            // If neither alphalock or shift produce characters,
            // use only one character code for this key,
            // but it can be a special character.
            } else {
                parse_next_char_code( i, keyMapStream );
                numKeyCodes--;
                while (numKeyCodes-- > 0) {     // skip the rest
                    get_number(keyMapStream); get_number(keyMapStream);
                
                }
            }
        }
    }

    // free Darwin keyboard map
    destroy_data_stream( keyMapStream );
    free( keyMap.mapping );
    NXCloseEventStatus( hidParam );
}


/*
 * ModifierKeycode
 *      Accessor function for modifier keycodes.
 */
int ModifierKeycode(
    int modifier,	// a NX_MODIFIERKEY_* code
    int side )		// 0 = left, 1 = right
{
    return modifierKeycodes[modifier][side];
}


/*
 * ArrowKeycode
 *      Accessor function for modifier keycodes.
 */
int ArrowKeycode(
    int a )		// arrow key number in Carbon keycode order
{
    return arrowKeycodes[a];
}
