#ifndef _sis_debug_h_
#define _sis_debug_h_

void dump_agp (void *addr, int dword_count);
void d2f (void);
void d2f_once (GLcontext * ctx);
void d2h (char *file_name);
void dvidmem (unsigned char *addr, int size);
extern char *IOBase4Debug;

#endif
