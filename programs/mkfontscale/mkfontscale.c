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
/* $XFree86$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include <X11/fonts/fontenc.h>
#include <freetype/freetype.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>
#include <freetype/ttnameid.h>
#include <freetype/t1tables.h>
#include "freetype/internal/ftobjs.h"
#include "freetype/internal/t1types.h"
#include "freetype/ftmodule.h"

#define FACE_TYPE(FACE) ((FACE)->driver->clazz->root.module_name)
#define T1INFO(FACE) \
  (strcasecmp(FACE_TYPE(FACE), "type1") == 0 ?\
   (&((T1_FaceRec*)FACE)->type1.font_info) : \
   NULL)
#define CODE_IGNORED(c) ((c) < 0x20 || \
                         ((c) >= 0x7F && (c) <= 0xA0) || \
                         (c) == 0xAD)
   
#include "list.h"
#include "data.h"

char *encodings_array[] =
    { "iso8859-1", "iso8859-2", "iso8859-3", "iso8859-4", "iso8859-5",
      "iso8859-6", "iso8859-7", "iso8859-8", "iso8859-9", "iso8859-10",
      "iso8859-11", "iso8859-12", "iso8859-13", "iso8859-14", "iso8859-15",
      "koi8-r", "koi8-u", "koi8-e",
      "adobe-standard", "adobe-symbol", "ibm-cp437", "microsoft-cp1252",
      "jisx0201.1976-0", "jisx0208.1983-0", "jisx0208.1990-0",
      "jisx0212.1190-0", "big5.eten-0", "gb2312.1980-0",
      "ksc5601.1987-0", "ksc5601.1992-3"};

char *forced_encodings_array[] =
    { "iso10646-1" };

char *extra_encodings_array[] =
    { "adobe-fontspecific", "microsoft-symbol" };

ListPtr encodings, forced_encodings, extra_encodings;

#define countof(_a) (sizeof(_a)/sizeof((_a)[0]))

int doDirectory(char*);
static int checkEncoding(FT_Face face, char *encoding_name, int force);
static int checkExtraEncoding(FT_Face face, char *encoding_name, int found);
static int find_cmap(int type, int pid, int eid, FT_Face face, 
                     FT_CharMap *cmap_return);
static char* notice_foundry(char *notice);
static char* vendor_foundry(char *vendor);

static FT_Library ft_library;

static void
usage(void)
{
    fprintf(stderr, 
            "mkfontscale [ -e encoding ] [ -f encoding ] [ directory ]\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    int argn;
    FT_Error ftrc;

    encodings = makeList(encodings_array, countof(encodings_array), NULL, 0);
    forced_encodings = makeList(forced_encodings_array, 
                                countof(forced_encodings_array),
                                NULL, 0);

    extra_encodings = makeList(extra_encodings_array, 
                               countof(extra_encodings_array),
                               NULL, 0);

    argn = 1;
    while(argn < argc) {
        if(argv[argn][0] == '\0' || argv[argn][0] != '-')
            break;
        if(argv[argn][1] == '-') {
            argn++;
            break;
        } else if(argv[argn][1] == 'e') {
            if(argn >= argc - 1)
                usage();
            makeList(&argv[argn + 1], 1, encodings, 0);
            argn += 2;
        } else if(argv[argn][1] == 'f') {
            if(argn >= argc - 1)
                usage();
            makeList(&argv[argn + 1], 1, forced_encodings, 0);
            argn += 2;
        } else {
            usage();
        }
    }

    ftrc = FT_Init_FreeType(&ft_library);
    if(ftrc) {
        fprintf(stderr, "Couldn't init library: %d\n", ftrc);
        exit(1);
    }
        

    if (argn == argc)
        doDirectory(".");
    else
        while(argn < argc) {
            doDirectory(argv[argn]);
            argn++;
        }
    return 0;
}

static int
getNameHelper(FT_Face face, int nid, int pid, int eid,
              FT_SfntName *name_return)
{
    FT_SfntName name;
    int n, i;

    n = FT_Get_Sfnt_Name_Count(face);
    if(n <= 0)
        return 0;

    for(i = 0; i < n; i++) {
        if(FT_Get_Sfnt_Name(face, i, &name))
            continue;
        if(name.name_id == nid &&
           name.platform_id == pid &&
           (eid < 0 || name.encoding_id == eid)) {
            switch(name.platform_id) {
            case TT_PLATFORM_APPLE_UNICODE:
            case TT_PLATFORM_MACINTOSH:
                if(name.language_id != TT_MAC_LANGID_ENGLISH)
                    continue;
                break;
            case TT_PLATFORM_MICROSOFT:
                if(name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES &&
                   name.language_id != TT_MS_LANGID_ENGLISH_UNITED_KINGDOM)
                    break;
                continue;
                break;
            default:
                break;
            }
            *name_return = name;
            return 1;
        }
    }
    return 0;
}

static char *
getName(FT_Face face, int nid)
{
    FT_SfntName name;
    char *string;
    int i;

    if(getNameHelper(face, nid, 
                     TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, &name) ||
       getNameHelper(face, nid, 
                     TT_PLATFORM_APPLE_UNICODE, -1, &name)) {
        string = malloc(name.string_len / 2 + 1);
        if(string == NULL) {
            fprintf(stderr, "Couldn't allocate name\n");
            exit(1);
        }
        for(i = 0; i < name.string_len / 2; i++) {
            if(name.string[2 * i])
                string[i] = '?';
            else
                string[i] = name.string[2 * i + 1];
        }
        string[i] = '\0';
        return string;
    }

    /* Pretend that Apple Roman is ISO 8859-1. */
    if(getNameHelper(face, nid, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN,
                     &name)) {
        string = malloc(name.string_len + 1);
        if(string == NULL) {
            fprintf(stderr, "Couldn't allocate name\n");
            exit(1);
        }
        memcpy(string, name.string, name.string_len);
        string[name.string_len] = '\0';
        return string;
    }

    return NULL;
}

static char*
os2Weight(int weight)
{
    if(weight < 150)
        return "thin";
    else if(weight < 250)
        return "extralight";
    else if(weight < 350)
        return "light";
    else if(weight < 450)
        return "medium";        /* officially "normal" */
    else if(weight < 550)
        return "medium";
    else if(weight < 650)
        return "semibold";
    else if(weight < 750)
        return "bold";
    else if(weight < 850)
        return "extrabold";
    else 
        return "black";
}

static char*
os2Width(int width)
{
    if(width <= 1)
        return "ultracondensed";
    else if(width <= 2)
        return "extracondensed";
    else if(width <= 3)
        return "condensed";
    else if(width <= 4)
        return "semicondensed";
    else if(width <= 5)
        return "normal";
    else if(width <= 6)
        return "semiexpanded";
    else if(width <= 7)
        return "expanded";
    else if(width <= 8)
        return "extraexpanded";
    else
        return "ultraexpanded";
}

static char*
t1Weight(char *weight)
{
    if(!weight)
        return NULL;
    if(strcmp(weight, "Regular") == 0)
        return "medium";
    if(strcmp(weight, "Normal") == 0)
        return "medium";
    if(strcmp(weight, "Medium") == 0)
        return "medium";
    else if(strcmp(weight, "Bold") == 0)
        return "bold";
    else {
        fprintf(stderr, "Unknown Type 1 weight %s\n", weight);
        return NULL;
    }
}

static char*
strcat_reliable(char *a, char *b) 
{
    char *c = malloc(strlen(a) + strlen(b) + 1);
    if(c == NULL)
        return NULL;
    strcpy(c, a);
    strcat(c, b);
    return c;
}

int
doDirectory(char *dirname_given)
{
    char *dirname, *fontscale_name, *filename;
    FILE *fontscale;
    DIR *dirp;
    struct dirent *entry;
    FT_Error ftrc;
    FT_Face face;
    TT_Header *head;
    TT_HoriHeader *hhea;
    TT_OS2 *os2;
    TT_Postscript *post;
    T1_FontInfo *t1info;
    char *foundry, *family, *weight, *slant, *sWidth, *adstyle, 
        *spacing;
    ListPtr encoding, entries = NULL;
    int i, found;

    i = strlen(dirname_given);
    if(i == 0)
        dirname = strcat_reliable(".", "/");
    else if(dirname_given[i - 1] != '/')
        dirname = strcat_reliable(dirname_given, "/");
    else
        dirname = strcat_reliable(dirname_given, "");
    fontscale_name = strcat_reliable(dirname, "fonts.scale");

    dirp = opendir(dirname);
    if(dirp == NULL) {
        fprintf(stderr, "%s: ", dirname);
        perror("opendir");
        return 0;
    }

    fontscale = fopen(fontscale_name, "w");
    if(fontscale == NULL) {
        fprintf(stderr, "%s: ", fontscale_name);
        perror("fopen(w)");
        return 0;
    }
    
    for(;;) {
        entry = readdir(dirp);
        if(entry == NULL)
            break;
        filename = strcat_reliable(dirname, entry->d_name);
        ftrc = FT_New_Face(ft_library, filename, 0, &face);
        if(ftrc)
            continue;
        found = 0;

        foundry = NULL;
        family = NULL;
        weight = NULL;
        slant = NULL;
        sWidth = NULL;
        adstyle = NULL;
        spacing = NULL;

        head = FT_Get_Sfnt_Table(face, ft_sfnt_head);
        hhea = FT_Get_Sfnt_Table(face, ft_sfnt_hhea);
        os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        post = FT_Get_Sfnt_Table(face, ft_sfnt_post);
        t1info = T1INFO(face);
        
        if(!family)
            family = getName(face, TT_NAME_ID_FONT_FAMILY);
        if(!family)
            family = getName(face, TT_NAME_ID_FULL_NAME);
        if(!family)
            family = getName(face, TT_NAME_ID_PS_NAME);

        if(os2 && os2->version != 0xFFFF) {
            if(!weight)
                weight = os2Weight(os2->usWeightClass);
            if(!sWidth)
                sWidth = os2Width(os2->usWidthClass);
            if(!foundry)
		/* os2->achVendID is exlicitly "signed" */
                foundry = vendor_foundry((char *)os2->achVendID);
            if(!slant)
                slant = os2->fsSelection & 1 ? "i" : "r";
        }

        if(post) {
            if(!spacing)
                spacing = post->isFixedPitch ? "m" : "p";
        }
            
        if(t1info) {
            if(!family)
                family = t1info->family_name;
            if(!foundry)
                foundry = notice_foundry(t1info->notice);
            if(!weight)
                weight = t1Weight(t1info->weight);
            if(!spacing)
                spacing = t1info->is_fixed_pitch ? "m" : "p";
            if(!slant) {
                /* Bitstream fonts have positive italic angle. */
                slant =
                    t1info->italic_angle <= -4 || t1info->italic_angle >= 4 ?
                    "i" : "r";
            }
        }

        if(head) {
            if(!slant)
                slant = head->Mac_Style & 2 ? "i" : "r";
            if(!weight)
                slant = head->Mac_Style & 1 ? "bold" : "medium";
        }

        if(!foundry) {
            char *notice;
            notice = getName(face, TT_NAME_ID_TRADEMARK);
            if(notice) {
                foundry = notice_foundry(notice);
            }
            if(!foundry) {
                notice = getName(face, TT_NAME_ID_MANUFACTURER);
                if(notice) {
                    foundry = notice_foundry(notice);
                }
            }
        }

        if(!foundry) foundry = "misc";
        if(!family) {
            fprintf(stderr, "Couldn't get family name for %s\n", filename);
            family = entry->d_name;
        }

        if(!weight) weight = "medium";
        if(!slant) slant = "r";
        if(!sWidth) sWidth = "normal";
        if(!adstyle) adstyle = "";
        if(!spacing) spacing = "p";

        for(encoding = forced_encodings; encoding; encoding = encoding->next)
            if(checkEncoding(face, encoding->value, 1)) {
                entries = listConsF(entries,
                                    "%s -%s-%s-%s-%s-%s-%s-0-0-0-0-%s-0-%s",
                                    entry->d_name,
                                    foundry, family, 
                                    weight, slant, sWidth, adstyle, spacing,
                                    encoding->value);
            }

        for(encoding = encodings; encoding; encoding = encoding->next)
            if(checkEncoding(face, encoding->value, 0)) {
                found = 1;
                entries = listConsF(entries,
                                    "%s -%s-%s-%s-%s-%s-%s-0-0-0-0-%s-0-%s",
                                    entry->d_name,
                                    foundry, family, 
                                    weight, slant, sWidth, adstyle, spacing,
                                    encoding->value);
            }
        for(encoding = extra_encodings; encoding; encoding = encoding->next)
            if(checkExtraEncoding(face, encoding->value, found)) {
                found = 1;
                entries = listConsF(entries,
                                    "%s -%s-%s-%s-%s-%s-%s-0-0-0-0-%s-0-%s",
                                    entry->d_name,
                                    foundry, family, 
                                    weight, slant, sWidth, adstyle, spacing,
                                    encoding->value);
            }
        free(filename);
    }
    entries = reverseList(entries);
    fprintf(fontscale, "%d\n", listLength(entries));
    while(entries) {
        fprintf(fontscale, "%s\n", entries->value);
        entries = entries->next;
    }
    deepDestroyList(entries);
    fclose(fontscale);
    free(fontscale_name);
    free(dirname);
    return 1;
}

static int
checkEncoding(FT_Face face, char *encoding_name, int force)
{
    FontEncPtr encoding;
    FontMapPtr mapping;
    int i, j, c, koi8;
    FT_CharMap cmap;

    encoding = FontEncFind(encoding_name, NULL);
    if(!encoding)
        return 0;

    for(mapping = encoding->mappings; mapping; mapping = mapping->next) {
        if(find_cmap(mapping->type, mapping->pid, mapping->eid, face, &cmap)) {
            if(force) {
                return 1;
            } else {
                if(encoding->row_size > 0) {
                    for(i = encoding->first; i < encoding->size; i++) {
                        for(j = encoding->first_col; 
                            j < encoding->row_size; 
                            j++) {
                            c = FontEncRecode((i<<8) | j, mapping);
                            if(CODE_IGNORED(c)) {
                                continue;
                            } else {
                                if(FT_Get_Char_Index(face, c) == 0) {
                                    return 0;
                                }
                            }
                        }
                    }
                    return 1;
                } else {
                    /* For the KOI8-* encodings, ignore the lack of
                       linedrawing characters */
                    if(strncmp(encoding->name, "koi8-", 5) == 0)
                        koi8 = 1;
                    else
                        koi8 = 0;
                    for(i = encoding->first; i < encoding->size; i++) {
                        c = FontEncRecode(i, mapping);
                        if(CODE_IGNORED(c) ||
                           (koi8 && i >= 0x80 && i < 0xA0)) {
                            continue;
                        } else {
                            if(FT_Get_Char_Index(face, c) == 0) {
                                return 0;
                            }
                        }
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int 
find_cmap(int type, int pid, int eid, FT_Face face, FT_CharMap *cmap_return)
{
    int i, n;
    FT_CharMap cmap = NULL;

    n = face->num_charmaps;

    switch(type) {
    case FONT_ENCODING_TRUETYPE:  /* specific cmap */
        for(i=0; i<n; i++) {
            cmap = face->charmaps[i];
            if(cmap->platform_id == pid && cmap->encoding_id == eid) {
                *cmap_return = cmap;
                return 1;
            }
        }
        break;
    case FONT_ENCODING_UNICODE:   /* any Unicode cmap */
        /* prefer Microsoft Unicode */
        for(i=0; i<n; i++) {
            cmap = face->charmaps[i];
            if(cmap->platform_id == TT_PLATFORM_MICROSOFT && 
               cmap->encoding_id == TT_MS_ID_UNICODE_CS) {
                *cmap_return = cmap;
                return 1;
            }
        }
        break;
        /* Try Apple Unicode */
        for(i=0; i<n; i++) {
            cmap = face->charmaps[i];
            if(cmap->platform_id == TT_PLATFORM_APPLE_UNICODE) {
                *cmap_return = cmap;
                return 1;
            }
        }
        /* ISO Unicode? */
        for(i=0; i<n; i++) {
            cmap = face->charmaps[i];
            if(cmap->platform_id == TT_PLATFORM_ISO) {
                *cmap_return = cmap;
                return 1;
            }
        }
        break;
    default:
        return 0;
    }
    return 0;
}

static int
checkExtraEncoding(FT_Face face, char *encoding_name, int found)
{
    FT_CharMap cmap;

    if(strcasecmp(encoding_name, "microsoft-symbol") == 0) {
        if(find_cmap(FONT_ENCODING_TRUETYPE,
                     TT_PLATFORM_MICROSOFT, TT_MS_ID_SYMBOL_CS,
                     face, &cmap))
            return 1;
        else
            return 0;
    } else if(strcasecmp(encoding_name, "adobe-fontspecific") == 0) {
        if(!found) {
            if(strcmp(FACE_TYPE(face), "type1") == 0)
                return 1;
            else
                return 0;
        } else
            return 0;
    }

    return 0;
}

static char*
notice_foundry(char *notice)
{
    int i;
    for(i = 0; i < countof(notice_foundries); i++)
        if(notice && strstr(notice, notice_foundries[i][0]))
            return notice_foundries[i][1];
    return NULL;
}

static int
vendor_match(char *vendor, char *vendor_string)
{
    /* vendor is not necessarily NUL-terminated. */
    int i, len;
    len = strlen(vendor_string);
    if(memcmp(vendor, vendor_string, len) != 0)
        return 0;
    for(i = len; i < 4; i++)
        if(vendor[i] != ' ' && vendor[i] != '\0')
            return 0;
    return 1;
}

static char*
vendor_foundry(char *vendor)
{
    int i;
    for(i = 0; i < countof(vendor_foundries); i++)
        if(vendor_match(vendor, vendor_foundries[i][0]))
            return vendor_foundries[i][1];
    return NULL;
}
