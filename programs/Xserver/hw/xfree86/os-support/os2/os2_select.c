/* $XFree86$ */

#include <sys/emx.h>
#include <stdlib.h>
#include <memory.h>
#include <io.h>
#include <sys/types.h>
#include <sys/time.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

int _select2 (int nfds, fd_set *readfds, fd_set *writefds,
             fd_set *exceptfds, struct timeval *timeout)
{
  struct _select args;
  int i, j, n;

  args.nfds = nfds;
  args.readfds = readfds;
  args.writefds = writefds;
  args.exceptfds = exceptfds;
  args.timeout = timeout;
  n = MIN (FD_SETSIZE, _nfiles);
  if (readfds != NULL)
    {
      for (i = 0; i < n; ++i) {
        if (FD_ISSET (i, readfds) && (!(_files[i] & F_PIPE)))
         {
            FD_CLR (i, readfds);    
         }
       }
    }

   if (writefds != NULL)
    {
      for (i = 0; i < n; ++i) {
        if (FD_ISSET (i, writefds) && (!(_files[i] & F_PIPE)))
         {
            FD_CLR (i, writefds);
         }
       }
    }
   if (exceptfds != NULL)
    {
      for (i = 0; i < n; ++i) {
        if (FD_ISSET (i, exceptfds) && (!(_files[i] & F_PIPE)))
         {
            FD_CLR (i, exceptfds);
         }
       }
    }

 return __select (&args);
}
