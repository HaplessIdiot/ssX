/*
Copyright (c) 2002-2003 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

#include "freetype/freetype.h"
#include "freetype/internal/ftobjs.h"
#include "freetype/bdf.h"
#include "freetype/bdfdrivr.h"
#include "freetype/pcf.h"
#include "fonttosfnt.h"

char*
sprintf_reliable(char *f, ...)
{
    char *s;
    va_list args;
    va_start(args, f);
    s = vsprintf_reliable(f, args);
    va_end(args);
    return s;
}    

char*
vsprintf_reliable(char *f, va_list args)
{
    int n, size = 12;
    char *string;
    while(1) {
        if(size > 4096)
            return NULL;
        string = malloc(size);
        if(!string)
            return NULL;
        n = vsnprintf(string, size, f, args);
        if(n >= 0 && n < size)
            return string;
        else if(n >= size)
            size = n + 1;
        else
            size = size * 3 / 2 + 1;
        free(string);
    }
    /* NOTREACHED */
}

/* Build a UTF-16 string from a Latin-1 string.  
   Result is not NUL-terminated. */
char *
makeUTF16(char *string)
{
    int i;
    int n = strlen(string);
    char *value = malloc(2 * n);
    if(!value)
        return NULL;
    for(i = 0; i < n; i++) {
        value[2 * i] = '\0';
        value[2 * i + 1] = string[i];
    }
    return value;
}

unsigned
makeName(char *s)
{
    return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}

/* Return the current time as a signed 64-bit number of seconds since
   midnight, 1 January 1904.  This is apparently when the Macintosh
   was designed. */
int
macTime(int *hi, unsigned *lo)
{
    time_t macEpoch, current;
    struct tm tm, *ltime;
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    tm.tm_mday = 1;
    tm.tm_mon = 1;
    tm.tm_year = 4;
    tm.tm_isdst = -1;

    macEpoch = mktime(&tm);
    if(macEpoch < 0) return -1;

    /* For some reason, mktime spits out local time */
    ltime = localtime(&macEpoch);
    macEpoch += ltime->tm_gmtoff;

    current = time(NULL);
    if(current < 0)
        return -1;

    if(current < macEpoch) {
        errno = EINVAL;
        return -1;
    }

    *hi = (current - macEpoch) >> 32;
    *lo = (current - macEpoch) & 0xFFFFFFFF;
    return 0;
}

static int
fontType(FT_Face face, char *name)
{
    if(face && face->driver) {
        FT_Module driver = (FT_Module)face->driver;
        if(driver->clazz && driver->clazz->module_name) {
            if(strcmp(driver->clazz->module_name, name) == 0)
                return 1;
        }
    }
    return 0;
}

static int
isX11Font(FT_Face face)
{
    return fontType(face, "pcf") || fontType(face, "bdf");
}

int
faceProperty(FT_Face face, char *name,
             int *type_return, FontProperty *prop_return)
{
    int i;
    if(fontType(face, "pcf")) {
        PCF_Face pf = (PCF_Face)face;
        for(i = 0; i < pf->nprops; i++) {
            if(strcmp(pf->properties[i].name, name) == 0) {
                if(type_return)
                    *type_return = 
                        pf->properties[i].isString ? PROP_ATOM : PROP_CARDINAL;
                *prop_return = *(FontProperty*)&pf->properties[i].value;
                return 1;
            }
        }
        return 0;
    } else if(fontType(face, "bdf")) {
        BDF_Face bf = (BDF_Face)face;
        for(i = 0; i < bf->bdffont->props_used; i++) {
            if(strcmp(bf->bdffont->props[i].name, name) == 0) {
                if(type_return)
                    *type_return = bf->bdffont->props[i].format;
                *prop_return = *(FontProperty*)&bf->bdffont->props[i].value;
                return 1;
            }
        }
        return 0;
    }
    return -1;
}

unsigned
faceFoundry(FT_Face face)
{
    int rc;
    if(isX11Font(face)) {
        FontProperty prop;
        rc = faceProperty(face, "FOUNDRY", NULL, &prop);
        if(rc > 0) {
            if(strcasecmp(prop.atom, "adobe") == 0)
                return makeName("ADBE");
            else if(strcasecmp(prop.atom, "agfa") == 0)
                return makeName("AGFA");
            else if(strcasecmp(prop.atom, "altsys") == 0)
                return makeName("ALTS");
            else if(strcasecmp(prop.atom, "apple") == 0)
                return makeName("APPL");
            else if(strcasecmp(prop.atom, "arphic") == 0)
                return makeName("ARPH");
            else if(strcasecmp(prop.atom, "alltype") == 0)
                return makeName("ATEC");
            else if(strcasecmp(prop.atom, "b&h") == 0)
                return makeName("B&H ");
            else if(strcasecmp(prop.atom, "bitstream") == 0)
                return makeName("BITS");
            else if(strcasecmp(prop.atom, "dynalab") == 0)
                return makeName("DYNA");
            else if(strcasecmp(prop.atom, "ibm") == 0)
                return makeName("IBM ");
            else if(strcasecmp(prop.atom, "itc") == 0)
                return makeName("ITC ");
            else if(strcasecmp(prop.atom, "interleaf") == 0)
                return makeName("LEAF");
            else if(strcasecmp(prop.atom, "impress") == 0)
                return makeName("IMPR");
            else if(strcasecmp(prop.atom, "larabiefonts") == 0)
                return makeName("LARA");
            else if(strcasecmp(prop.atom, "linotype") == 0)
                return makeName("LINO");
            else if(strcasecmp(prop.atom, "monotype") == 0)
                return makeName("MT  ");
            else if(strcasecmp(prop.atom, "microsoft") == 0)
                return makeName("MS  ");
            else if(strcasecmp(prop.atom, "urw") == 0)
                return makeName("URW ");
            else if(strcasecmp(prop.atom, "y&y") == 0)
                return makeName("Y&Y ");
            else
                return makeName("UNKN");
        }
        return makeName("UNKN");
    } else {
        return makeName("UNKN"); /* for now */
    }
}
    

int
faceWeight(FT_Face face)
{
    int rc;
    if(isX11Font(face)) {
        FontProperty prop;
        rc = faceProperty(face, "WEIGHT_NAME", NULL, &prop);
        if(rc > 0) {
            if(strcasecmp(prop.atom, "thin") == 0)
                return 100;
            else if(strcasecmp(prop.atom, "extralight") == 0)
                return 200;
            else if(strcasecmp(prop.atom, "light") == 0)
                return 300;
            else if(strcasecmp(prop.atom, "medium") == 0)
                return 500;
            else if(strcasecmp(prop.atom, "semibold") == 0)
                return 600;
            else if(strcasecmp(prop.atom, "bold") == 0)
                return 700;
            else if(strcasecmp(prop.atom, "extrabold") == 0)
                return 800;
            else if(strcasecmp(prop.atom, "black") == 0)
                return 900;
            else
                return 500;
        } else
            return 500;
    } else {
        return 500;             /* for now */
    }
}

int
faceWidth(FT_Face face)
{
    int rc;
    if(isX11Font(face)) {
        FontProperty prop;
        rc = faceProperty(face, "SETWIDTH_NAME", NULL, &prop);
        if(rc > 0) {
            if(strcasecmp(prop.atom, "ultracondensed") == 0)
                return 1;
            else if(strcasecmp(prop.atom, "extracondensed") == 0)
                return 2;
            else if(strcasecmp(prop.atom, "condensed") == 0)
                return 3;
            else if(strcasecmp(prop.atom, "semicondensed") == 0)
                return 4;
            else if(strcasecmp(prop.atom, "normal") == 0)
                return 5;
            else if(strcasecmp(prop.atom, "semiexpanded") == 0)
                return 6;
            else if(strcasecmp(prop.atom, "expanded") == 0)
                return 7;
            else if(strcasecmp(prop.atom, "extraexpanded") == 0)
                return 8;
            else if(strcasecmp(prop.atom, "ultraexpanded") == 0)
                return 9;
            else
                return 5;

        } else
            return 5;
    } else {
        return 5;               /* for now */
    }
}

int
faceItalicAngle(FT_Face face)
{
    int rc;
    if(isX11Font(face)) {
        FontProperty prop;
        rc = faceProperty(face, "ITALIC_ANGLE", NULL, &prop);
        if(rc > 0) {
            return (prop.int32 - 64 * 90) * (TWO_SIXTEENTH / 64);
        }
        rc = faceProperty(face, "SLANT", NULL, &prop);
        if(rc > 0) {
            if(strcasecmp(prop.atom, "i") == 0 ||
               strcasecmp(prop.atom, "s") == 0)
                return -30 * TWO_SIXTEENTH;
            else
                return 0;
        } else
            return 0;
    } else {
        return 0;               /* for now */
    }
}

int
faceFlags(FT_Face face)
{
    int flags = 0;
    int rc;

    if(isX11Font(face)) {
        FontProperty prop;
        if(faceWeight(face) >= 650)
            flags |= FACE_BOLD;
        rc = faceProperty(face, "SLANT", NULL, &prop);
        if(rc > 0) {
            if(strcasecmp(prop.atom, "i") == 0 ||
               strcasecmp(prop.atom, "s") == 0)
                flags |= FACE_ITALIC;
        }
    } else {
        ;                       /* for now */
    }
    return flags;
}

int
degreesToFraction(int deg, int *num, int *den)
{
    double n, d;
    double rad, val;
    int i;

    if(deg <= -(60 * TWO_SIXTEENTH) || deg >= (60 * TWO_SIXTEENTH))
        goto fail;

    rad = (((double)deg) / TWO_SIXTEENTH) / 180.0 * M_PI;

    n = sin(-rad);
    d = cos(rad);

    if(d < 0.001)
        goto fail;

    val = atan2(n, d);
    /* There must be a cleaner way */
    for(i = 1; i < 10000; i++) {
        if((int)(d * i) != 0.0 &&
           fabs(atan2(ROUND(n * i), ROUND(d * i)) - val) < 0.05) {
            *num = (int)ROUND(n * i);
            *den = (int)ROUND(d * i);
            return 0;
        }
    }

 fail:
    *den = 1;
    *num = 0;
    return -1;
}

