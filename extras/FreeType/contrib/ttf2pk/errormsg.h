/*
 *   errormsg.h
 *
 *   This file is part of the ttf2pk package.
 *
 *   Copyright 1997-1998 by
 *     Frederic Loyer <loyer@ensta.fr>
 *     Werner Lemberg <wl@gnu.org>
 */

#ifndef ERRORMSG_H
#define ERRORMSG_H

void oops(const char *message,
          ...);
void boops(const char *buffer,
           size_t offset,
           const char *message,
           ...);
void warning(const char *message,
             ...);

#endif /* ERRORMSG_H */


/* end */
