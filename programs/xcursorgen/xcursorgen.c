/*
 * xcursorgen.c
 *
 * Copyright (C) 2002 Manish Singh
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Manish Singh not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Manish Singh makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MANISH SINGH DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

#include <png.h>

#define VERSION_STR "0.1"

struct flist
{
  int size;
  int xhot, yhot;
  char *pngfile;
  struct flist *next;
};

static void
usage (char *name)
{
  fprintf (stderr, "usage: %s [-Vh] [--version] [--help] [CONFIG [OUT]]\n",
	   name);

  fprintf (stderr, "Generate an Xcursor file from a series of PNG images\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "  -V, --version    display the version number and exit\n");
  fprintf (stderr, "  -?, --help       display this message and exit\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "With no CONFIG, or when CONFIG is -, read standard input. "
		   "Same with OUT and\n");
  fprintf (stderr, "standard output.\n");
}

static int
read_config_file (char *config, struct flist **list)
{
  FILE *fp;
  char line[4096], pngfile[4000];
  int size, xhot, yhot;
  struct flist *start = NULL, *end, *curr;
  int count = 0;

  if (strcmp (config, "-") != 0)
    {
      fp = fopen (config, "r");
      if (!fp)
	{
	  *list = NULL;
          return 0;
	}
    }
  else
    fp = stdin;

  while (fgets (line, sizeof (line), fp) != NULL)
    {
      if (line[0] == '#')
	continue;

      if (sscanf (line, "%d %d %d %3999s", &size, &xhot, &yhot, pngfile) != 4)
	{
	  fprintf (stderr, "Bad config file data!\n");
	  fclose (fp);
	  return 0;
	}

      curr = malloc (sizeof (struct flist));

      curr->size = size;
      curr->xhot = xhot;
      curr->yhot = yhot;

      curr->pngfile = malloc (strlen (pngfile) + 1);
      strcpy (curr->pngfile, pngfile);

      curr->next = NULL;

      if (start)
	{
	  end->next = curr;
          end = curr;
	}
      else
	{
	  start = curr; 
          end = curr;
        }

      count++;
    }

  fclose (fp);

  *list = start;
  return count;
}

static void
premultiply_data (png_structp png, png_row_infop row_info, png_bytep data)
{
  int i;

  for (i = 0; i < row_info->rowbytes; i += 4)
    {
      unsigned char *b = &data[i];
      unsigned char alpha = b[3];
      XcursorPixel pixel = ((((b[0] * alpha) / 255) << 0) |
			    (((b[1] * alpha) / 255) << 8) |
			    (((b[2] * alpha) / 255) << 16) |
			    (alpha << 24));
      XcursorPixel *p = (XcursorPixel *) b;
      *p = pixel;
    }
}

static XcursorImage *
load_image (struct flist *list)
{
  XcursorImage *image;
  png_structp png;
  png_infop info;
  png_bytepp rows;
  FILE *fp;
  int i;
  png_uint_32 width, height;
  int depth, color, interlace;

  png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
    return NULL;

  info = png_create_info_struct (png);
  if (info == NULL)
    {
      png_destroy_read_struct (&png, NULL, NULL);
      return NULL;
    }

  if (setjmp (png->jmpbuf))
    {
      png_destroy_read_struct (&png, &info, NULL);
      return NULL;
    }

  fp = fopen (list->pngfile, "rb");
  if (fp == NULL)
    {
      png_destroy_read_struct (&png, &info, NULL);
      return NULL;
    }

  png_init_io (png, fp);
  png_read_info (png, info);
  png_get_IHDR (png, info, &width, &height, &depth, &color, &interlace,
		NULL, NULL);

  /* TODO: More needs to be done here maybe */

  if (color == PNG_COLOR_TYPE_PALETTE && depth <= 8)
    png_set_expand (png);

  if (color == PNG_COLOR_TYPE_GRAY && depth < 8)
    png_set_expand (png);

  if (png_get_valid (png, info, PNG_INFO_tRNS))
    png_set_expand (png);

  if (depth == 16)
    png_set_strip_16 (png);

  if (depth < 8)
    png_set_packing (png);

  if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png);

  if (interlace != PNG_INTERLACE_NONE)
    png_set_interlace_handling (png);

  png_set_bgr (png);
  png_set_filler (png, 255, PNG_FILLER_AFTER);

  png_set_read_user_transform_fn (png, premultiply_data);

  png_read_update_info (png, info);

  image = XcursorImageCreate (width, height);

  image->size = list->size;
  image->xhot = list->xhot;
  image->yhot = list->yhot;

  rows = malloc (sizeof (png_bytep) * height);
  
  for (i = 0; i < height; i++)
    rows[i] = (png_bytep) (image->pixels + i * width);

  png_read_image (png, rows);
  png_read_end (png, info);

  free (rows);
  fclose (fp);
  png_destroy_read_struct (&png, &info, NULL);

  return image;
}

static int
write_cursors (int count, struct flist *list, char *filename)
{
  XcursorImages *cimages;
  XcursorImage *image;
  int i;
  FILE *fp;
  int ret;

  if (strcmp (filename, "-") != 0)
    {
      fp = fopen (filename, "wb");
      if (!fp)
        return 1;
    }
  else
    fp = stdout;

  cimages = XcursorImagesCreate (count);

  cimages->nimage = count;

  for (i = 0; i < count; i++, list = list->next)
    {
      image = load_image (list);
      if (image == NULL)
	{
	  fprintf (stderr, "PNG error while reading %s!\n", list->pngfile);
	  return 1;
	}

      cimages->images[i] = image;
    }

  ret = XcursorFileSaveImages (fp, cimages);

  fclose (fp);

  return ret ? 0 : 1;
}

int
main (int argc, char *argv[])
{
  struct flist *list;
  int count;
  char *filename;

  if (argc > 1)
    {
      if (strcmp (argv[1], "-V") == 0 || strcmp (argv[1], "--version") == 0)
        {
          printf ("xcursorgen version %s\n", VERSION_STR);
          return 0;
        }

      if (strcmp (argv[1], "-?") == 0 || strcmp (argv[1], "--help") == 0)
        {
          usage (argv[0]);
          return 0;
        }

      filename = argv[1];
    }
  else
    filename = "-";

  count = read_config_file (filename, &list);
  if (count == 0)
    {
      fprintf (stderr, "Error reading config file!\n");
      return 1;
    }

  if (argc > 2)
    filename = argv[2];
  else
    filename = "-";

  return write_cursors (count, list, filename);
}
