/* Rexx OS/2
 * This script serves as a helper cmd file for imake. Install this in
 * the path just like imake itself.
 *
 * $XFree86: xc/config/imake/imakesvc.cmd,v 3.0 1994/12/17 09:33:44 dawes Exp $ */
 */
'@echo off'
call RxFuncAdd 'SysFileDelete', 'RexxUtil', 'SysFileDelete'
call RxFuncAdd 'SysFileTree', 'RexxUtil', 'SysFileTree'
call RxFuncAdd 'SysRmDir', 'RexxUtil', 'SysRmDir'

PARSE ARG all
code = WORD(all,1)

SELECT
   WHEN code=1 THEN DO
      /* imakesvc 1 u/n dir ruledir top current */
      instflg = WORD(all,2)
      imakecmd = '\imake'
      IF instflg = 'u' THEN imakecmd = 'imake'
      curdir = DIRECTORY()
      dir = WORD(all,3)
      d = DIRECTORY(dir)
      RC = SysFileDelete('Makefile.bak')
      IF exists('Makefile')=0 THEN REN Makefile Makefile.bak
      pfx = levels(TRANSLATE(dir,'/','\'))
      imakecmd '-I'pfx''WORD(all,4) '-DTOPDIR='pfx''WORD(all,5)' -DCURDIR='pfx''WORD(all,6)'/'dir
      'make SHELL= Makefiles'
      d = DIRECTORY(curdir)
   END
   WHEN code=2 THEN DO
      /* imakesvc 2 buildincdir buildinctop currentdir file */
      bid = WORD(all,3)
      cid = WORD(all,4)
      fil = WORD(all,5)
      curdir = DIRECTORY()
      d = DIRECTORY(WORD(all,2))
      rc = SysFileDelete(fil)
      dir = TRANSLATE(bid'/'cid'/'fil,'\','/')
      COPY dir .
      d = DIRECTORY(curdir)
   END
   WHEN code=3 THEN DO
      /* imakesvc 3 subdir updir file */
      sdi = WORD(all,2)
      fil = WORD(all,4)
      curdir = DIRECTORY()
      d = DIRECTORY(WORD(all,3))
      rc = SysFileDelete(fil)
      dir = TRANSLATE(sdi'/'fil,'\','/')
      COPY dir .
      d = DIRECTORY(curdir)
   END
   WHEN code=4 THEN DO
      /* imakesvc 4 [-r] files... */
      rec = WORD(all,2)
      fp = 2
      IF rec = '-r' THEN fp = 3 
      DO i=fp TO WORDS(all)
	 CALL discard rec TRANSLATE(WORD(all,i),'\','/')
      END	
   END
   WHEN code=5 THEN DO
      /* imakesvc 5 file */
      file = TRANSLATE(WORD(all,2),'\','/')
      if exists(file) THEN REN file file.bak
   END
   WHEN code=6 THEN DO
      /* imakesvc 6 file */
      file = TRANSLATE(WORD(all,2),'\','/')
      CALL SysFileDelete(file'.bak')
      if exists(file) THEN REN file file.bak
   END
   WHEN code=7 THEN DO
      /* imakesvc 7 from to */
      from = TRANSLATE(WORD(all,2),'\','/')
      to = TRANSLATE(WORD(all,3),'\','/')
      CALL SysFileDelete(to)
      COPY from to' 2> nul > nul'
   END
   WHEN code=8 THEN DO
      /* imakesvc 8 arg */
      SAY SUBWORD(TRANSLATE(all,'  ','222c'x),2)
   END
   WHEN code=9 THEN DO
      /* imakesvc 9 dst.c incl.h src.c */
      dst = TRANSLATE(WORD(all,2),'\','/')
      src = TRANSLATE(WORD(all,4),'\','/')
      CALL SysFileDelete(dst)
      CALL LINEOUT dst,'#include "'WORD(all,3)'"'
      CALL LINEOUT dst,'#include "'src'"'
      CALL LINEOUT dst 
   END
   OTHERWISE NOP
END
RETURN

levels:
oldpos = 1
pfx = ''
DO FOREVER
   newpos = POS('/',ARG(1),oldpos)
   IF newpos = 0 THEN LEAVE
   newpfx = '../'pfx
   oldpos = newpos+1
   pfx = newpfx
END
RETURN pfx

exists:
'DIR 'arg(1)' > nul 2>nul'
IF rc = 0 THEN return 0
RETURN 1

discard: PROCEDURE
arg rec files
IF rec = '-R' THEN DO
   old = DIRECTORY()
   nd = DIRECTORY
   CALL SysFileTree files, 'deld', 'DO'
   IF deld.0 > 0 THEN DO
      DO m=1 TO deld.0
         CALL DIRECTORY deld.m
         CALL discard '-R' .
         CALL DIRECTORY ..
         CALL SysRmDir deld.m
      END 
      CALL SysRmDir files
   END
   CALL SysFileTree files, 'delf', 'FO'
   DO k=1 TO delf.0
      DEL '"'delf.k'"' '> nul 2> nul'
   END
   CALL SysRmDir files
END 
ELSE DO
   DEL '"'files'"' '> nul 2> nul'
END
RETURN
