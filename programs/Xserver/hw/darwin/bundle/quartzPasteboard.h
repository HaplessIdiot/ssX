/* 
   QuartzPasteboard.h

   Mac OS X pasteboard <-> X cut buffer
   Greg Parker     gparker@cs.stanford.edu     March 8, 2001
*/
/* $XFree86: $ */

#ifndef _QUARTZPASTEBOARD_H
#define _QUARTZPASTEBOARD_H

// Aqua->X 
void QuartzReadPasteboard();
char * QuartzReadCocoaPasteboard(void);	// caller must free string

// X->Aqua
void QuartzWritePasteboard();
void QuartzWriteCocoaPasteboard(char *text);

#endif	/* _QUARTZPASTEBOARD_H */