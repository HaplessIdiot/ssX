/* $XFree86$ */
/*
 * Copyright 1994, Erik Nygren (nygren@mit.edu)
 *
 * This code may be freely incorporated in any program without royalty, as
 * long as the copyright notice stays intact.
 *
 * Additions by Harry Langenbacher (harry@brain.jpl.nasa.gov)
 *
 * ERIK NYGREN AND HARRY LANGENBACHER
 * DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL ERIK NYGREN
 * OR HARRY LANGENBACHER BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#define ORCHID_OCR_RESERVED_MASK     0xCF
#define ORCHID_OCR_ENABLE_P9000      0x10
#define ORCHID_OCR_ENABLE_W5186      0x00
#define ORCHID_OCR_SYNC_POSITIVE     0x20
#define ORCHID_OCR_SYNC_NEGATIVE     0x00

/* The location and text of the Orchid P9000 signature for autodetection */
#define ORCHID_BIOS_OFFSET           0x37
#define ORCHID_BIOS_LENGTH           54
/* The version isn't relavant */
#define ORCHID_BIOS_SIGNATURE        "I have no idea what goes here"  

extern p9000VendorRec p9000OrchidVendor;

