/*
 * Copyright 1997 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86$ */

#ifndef _XF86_ANSIC_H
#define _XF86_ANSIC_H

#ifdef XFree86LOADER
/*
 * at this point I don't think we support any non-ANSI compilers...
 */

/* ANSI C emulation library */

extern void xf86abort(void);
extern int xf86abs(int);
extern double xf86acos(double);
extern double xf86asin(double);
extern double xf86atan(double);
extern double xf86atan2(double,double);
extern double xf86atof(const char*);
extern int xf86atoi(const char*);
extern long xf86atol(const char*);
extern double xf86ceil(double);
extern void* xf86calloc(INT32,INT32);
extern void xf86clearerr(XF86FILE*);
extern double xf86cos(double);
extern void xf86exit(int);
extern double xf86exp(double);
extern double xf86fabs(double);
extern int xf86fclose(XF86FILE*);
extern int xf86feof(XF86FILE*);
extern int xf86ferror(XF86FILE*);
extern int xf86fflush(XF86FILE*);
extern int xf86fgetc(XF86FILE*);
extern int xf86fgetpos(XF86FILE*,XF86fpos_t*);
extern char* xf86fgets(char*,int,XF86FILE*);
extern double xf86floor(double);
extern double xf86fmod(double,double);
extern XF86FILE* xf86fopen(const char*,const char*);
extern int xf86fprintf(/*XF86FILE*,const char*,...*/);
extern int xf86fputc(int,XF86FILE*);
extern int xf86fputs(const char*,XF86FILE*);
extern INT32 xf86fread(void*,INT32,INT32,XF86FILE*);
extern void xf86free(void*);
extern XF86FILE* xf86freopen(const char*,const char*,XF86FILE*);
extern int xf86fscanf(/*XF86FILE*,const char*,...*/);
extern int xf86fseek(XF86FILE*,long,int);
extern int xf86fsetpos(XF86FILE*,const XF86fpos_t*);
extern long xf86ftell(XF86FILE*);
extern INT32 xf86fwrite(const void*,INT32,INT32,XF86FILE*);
extern char* xf86getenv(const char*);
extern int xf86isalnum(int);
extern int xf86isalpha(int);
extern int xf86iscntrl(int);
extern int xf86isdigit(int);
extern int xf86isgraph(int);
extern int xf86islower(int);
extern int xf86isprint(int);
extern int xf86ispunct(int);
extern int xf86isspace(int);
extern int xf86isupper(int);
extern int xf86isxdigit(int);
extern long xf86labs(long);
extern double xf86log(double);
extern double xf86log10(double);
extern void* xf86malloc(INT32);
extern void* xf86memchr(const void*,int,INT32);
extern int xf86memcmp(const void*,const void*,INT32);
extern int xf86memcpy(void*,const void*,INT32);
extern void* xf86memmove(void*,const void*,INT32);
extern void* xf86memset(void*,int,INT32);
extern double xf86modf(double,double*);
extern void xf86perror(const char*);
extern double xf86pow(double,double);
extern void* xf86realloc(void*,INT32);
extern int xf86remove(const char*);
extern int xf86rename(const char*,const char*);
extern void xf86rewind(XF86FILE*);
extern int xf86setbuf(XF86FILE*,char*);
extern int xf86setvbuf(XF86FILE*,char*,int,INT32);
extern double xf86sin(double);
extern int xf86sprintf(/*char*,const char*,...*/);
extern double xf86sqrt(double);
extern int xf86sscanf(/*const char* const char*,...*/);
extern char* xf86strcat(char*,const char*);
extern int xf86strcmp(const char*,const char*);
extern char* xf86strcpy(char*,const char*);
extern INT32 xf86strcspn(const char*,const char*);
extern char* xf86strerror(int);
extern INT32 xf86strlen(const char*);
extern int xf86strncmp(const char*,const char*,INT32);
extern char* xf86strncpy(char*,const char*,INT32);
extern char* xf86strpbrk(const char*,const char*);
extern char* xf86strrchr(const char*,int);
extern INT32 xf86strspn(const char*,const char*);
extern char* xf86strstr(const char*,const char*);
extern double xf86strtod(const char*,char**);
extern char* xf86strtok(char*,const char*);
extern long xf86strtol(const char*,char**,int);
extern unsigned long xf86strtoul(const char*,char**,int);
extern double xf86tan(double);
extern XF86FILE* xf86tmpfile(void);
extern char* xf86tmpnam(char*);
extern int xf86tolower(int);
extern int xf86toupper(int);
extern int xf86ungetc(int,XF86FILE*);
extern int xf86vfprintf(/*XF86FILE*,const char*,...*/);
extern int xf86vsprintf(/*char*,const char*,...*/);

/* non-ANSI C functions */
extern XF86DIR* xf86opendir(char*);
extern int xf86closedir(XF86DIR*);
extern XF86DIRENT* xf86readdir(XF86DIR*);
extern void xf86rewinddir(XF86DIR*);
extern void xf86bcopy(const void*,void*,INT32);
extern int xf86ffs(int);
extern char* xf86strdup(const char*);
extern void xf86bzero(void*,unsigned int);
extern void xf86usleep(unsigned long);
extern void xf86getsecs(INT32 *, INT32 *);
extern int xf86getbitsperpixel(int);
extern Bool xf86setexternclock(char *, int, int);
extern int xf86execl();
extern long xf86fpossize();

#endif /* XFree86LOADER */
  
#endif /* _XF86_ANSIC_H */
