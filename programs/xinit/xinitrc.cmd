/* OS/2 REXX */
/* $XFree86: xc/programs/xinit/xinitrc.cmd,v 3.0 1996/02/09 08:22:54 dawes Exp $ */
'@echo off'
env = 'OS2ENVIRONMENT'
x11root = VALUE('X11ROOT',,env)
IF x11root = '' THEN DO
	SAY "The environment variable X11ROOT is not set. XFree86/OS2 won't run without it."
	EXIT
END
home = VALUE('HOME',,env)
IF home = '' THEN home = x11root

userresources = home'\.Xresources'
usermodmap    = home'\.Xmodmap'
sysresources  = x11root'\XFree86\lib\X11\xinit\.Xresources'
sysmodmap     = x11root'\XFree86\lib\X11\xinit\.Xmodmap'

/* merge in defaults */

IF exists(sysresources) THEN
	xrdb -merge sysresources

IF exists(sysmodmap) THEN
	xmodmap sysmodmap

IF exists(userresources) THEN
	xrdb -merge userresources

IF exists(usermodmap) THEN
	xmodmap usermodmap

/* start some nice programs */
'start/min/n twm'
'start/min/n xclock -update 1 -geometry 100x100-1+1'
/* 'start/min/n 'xterm -sb -geometry 80x50+494+51 */
/* 'start/min/n 'xterm -sb -geometry 80x20+494-0 */
'xterm -sb -geometry 80x66+0+0 -name login'

EXIT

exists:
'DIR "'arg(1)'" >nul 2>&1'
if rc = 0 THEN RETURN 1
RETURN 0
