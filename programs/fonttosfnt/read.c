/*
Copyright (c) 2002 by Juliusz Chroboczek

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

#include <stdio.h>

#include "freetype/freetype.h"
#include "fonttosfnt.h"

#define FLOOR(x, y) ((x) >= 0 ? (x) / (y) : -(((-(x)) + (y) - 1) / (y)))
#define CEIL(x, y) (FLOOR((x) + (y) - 1, (y)))
#define FT_Pos_DOWN(x) (FLOOR((x),64))
#define FT_Pos_UP(x) (CEIL((x), 64))

static int ft_inited = 0;
static FT_Library ft_library;

static int
FT_Ensure_Inited()
{
    int rc;
    if(ft_inited)
        return 0;

    rc = FT_Init_FreeType(&ft_library);
    if(rc != 0)
        return rc;

    ft_inited = 1;
    return 0;
}

int
readFile(char *filename, FontPtr font)
{
    int i, j, index;
    int rc;
    FT_Face face;
    StrikePtr strike;
    BitmapPtr bitmap;

    rc = FT_Ensure_Inited();
    if(rc != 0)
        return rc;

    rc = FT_New_Face(ft_library, filename, 0, &face);
    if(rc != 0) {
        fprintf(stderr, "Couldn't open face %s.\n", filename);
        return -1;
    }

    if(verbose_flag) {
        fprintf(stderr, "%s %s %s: %d sizes\n",
                filename, face->family_name, face->style_name, 
                face->num_fixed_sizes);
    }

    if(font->numNames == 0 && face->style_name && face->family_name) {
        char *full_name, *unique_name;
        FontProperty prop;
        int rc, i;
        if(strcmp(face->style_name, "Regular") == 0)
            full_name = sprintf_reliable("%s", face->family_name);
        else
            full_name = sprintf_reliable("%s %s", 
                                         face->family_name, face->style_name);

        /* The unique name doesn't actually need to be globally
           unique; it only needs to be unique among all installed fonts on a
           Windows system.  We don't bother getting it quite right. */
        if(face->num_fixed_sizes <= 0)
            unique_name = sprintf_reliable("%s XFree86 bitmap", full_name);
        else if(face->available_sizes[0].width == 
                face->available_sizes[0].height)
            unique_name = sprintf_reliable("%s XFree86 bitmap size %d",
                                           full_name, 
                                           face->available_sizes[0].height);
        else
            unique_name = sprintf_reliable("%s XFree86 bitmap size %dx%d",
                                           full_name, 
                                           face->available_sizes[0].width,
                                           face->available_sizes[0].height);

        font->names = malloc(10 * sizeof(FontNameEntryRec));
        if(font->names == NULL) {
            fprintf(stderr, "Couldn't allocate names.\n");
            return -1;
        }
        i = 0;

        rc = faceProperty(face, "COPYRIGHT", NULL, &prop);
        if(rc > 0) {
            font->names[i].nid = 0;
            font->names[i].size = 2 * strlen(prop.atom);
            font->names[i].value = makeUTF16(prop.atom);
            i++;
        }

        font->names[i].nid = 1;
        font->names[i].size = 2 * strlen(face->family_name);
        font->names[i].value = makeUTF16(face->family_name);
        i++;

        font->names[i].nid = 2;
        font->names[i].size = 2 * strlen(face->style_name);
        font->names[i].value = makeUTF16(face->style_name);
        i++;

        font->names[i].nid = 3;
        font->names[i].size = 2 * strlen(unique_name);
        font->names[i].value = makeUTF16(unique_name);
        i++;

        font->names[i].nid = 4;
        font->names[i].size = 2 * strlen(full_name);
        font->names[i].value = makeUTF16(full_name);
        i++;

        font->names[i].nid = 5;
        font->names[i].size = 2 * strlen("Version 0.0");
        font->names[i].value = makeUTF16("Version 0.0");
        i++;

        rc = faceProperty(face, "FOUNDRY", NULL, &prop);
        if(rc > 0) {
            font->names[i].nid = 8;
            font->names[i].size = 2 * strlen(prop.atom);
            font->names[i].value = makeUTF16(prop.atom);
            i++;
        }

        font->names[i].nid = 10;
        font->names[i].size = 2 * strlen("XFree86 converted bitmap font");
        font->names[i].value = makeUTF16("XFree86 converted bitmap font");
        i++;

        font->names[i].nid = 11;
        font->names[i].size = 2 * strlen("http://www.xfree86.org");
        font->names[i].value = makeUTF16("http://www.xfree86.org");
        i++;

        font->numNames = i;

        font->flags = faceFlags(face);
        font->weight = faceWeight(face);
        font->width = faceWidth(face);
        font->foundry = faceFoundry(face);
        font->italicAngle = faceItalicAngle(face);

        rc = faceProperty(face, "UNDERLINE_POSITION", NULL, &prop);
        if(rc > 0)
            font->underlinePosition = 
                (double)prop.int32 / face->available_sizes[0].height *
                TWO_SIXTEENTH;
        else
            font->underlinePosition =
                - 1.5 / face->available_sizes[0].height * TWO_SIXTEENTH;

        rc = faceProperty(face, "UNDERLINE_THICKNESS", NULL, &prop);
        if(rc > 0)
            font->underlineThickness = 
                (double)prop.int32 / face->available_sizes[0].height *
                TWO_SIXTEENTH;
        else
            font->underlineThickness =
                1.0 / face->available_sizes[0].height * TWO_SIXTEENTH;
    }

    if(face->num_fixed_sizes == 0) {
        fprintf(stderr, "No bitmaps in face.\n");
        return -1;
    }
    
    rc = FT_Select_Charmap(face, ft_encoding_unicode);
    if(rc != 0) {
        fprintf(stderr, "Couldn't select character map: %x.\n", rc);
        return -1;
    }

    for(i = 0; i < face->num_fixed_sizes; i++) {
        if(verbose_flag)
            fprintf(stderr, "size %d: %dx%d\n",
                    i, 
                    face->available_sizes[i].width, 
                    face->available_sizes[i].height);

        rc = FT_Set_Pixel_Sizes(face,
                                face->available_sizes[i].width,
                                face->available_sizes[i].height);
        if(rc != 0) {
            fprintf(stderr, "Couldn't set size.\n");
            return -1;
        }

        strike = makeStrike(font, 
                            face->available_sizes[i].width,
                            face->available_sizes[i].height);
        if(strike == NULL) {
            fprintf(stderr, "Couldn't allocate strike.\n");
            return -1;
        }

        for(j = 0; j < FONT_CODES; j++) {
            index = FT_Get_Char_Index(face, j);
            if(j != 0 && index == 0)
                continue;
            rc = FT_Load_Glyph(face, index,
                               FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
            if(rc != 0) {
                fprintf(stderr, "Couldn't load glyph for U+%04X (%d)\n",
                        j, index);
                continue;
            }
            bitmap = makeBitmap(strike, j,
                                FT_Pos_UP(face->glyph->metrics.horiAdvance),
                                FT_Pos_DOWN(face->glyph->metrics.horiBearingX),
                                FT_Pos_DOWN(face->glyph->metrics.horiBearingY),
                                face->glyph->bitmap.width,
                                face->glyph->bitmap.rows,
                                face->glyph->bitmap.pitch, 
                                face->glyph->bitmap.buffer,
                                crop_flag);

            if(bitmap == NULL) {
                fprintf(stderr, "Couldn't create bitmap.\n");
                return -1;
            }
        }
    }

    FT_Done_Face(face);

    j = 0;
    for(i = 0; i < FONT_CODES; i++) {
        int found = 0;
        strike = font->strikes;
        while(strike) {
            bitmap = STRIKE_BITMAP(strike, i);
            if(bitmap) {
                bitmap->index = j;
                found = 1;
            } else {
                if(i == 0) {
                    fprintf(stderr, 
                            "Warning: no bitmap for the undefined glyph.\n");
                    found = 1;
                }
            }
            strike = strike->next;
        }
        if(found)
            j++;
    }
    return 0;
}
