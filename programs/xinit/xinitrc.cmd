/* OS/2 REXX */
/* $XFree86$ */
'@echo off'
env = 'OS2ENVIRONMENT'
x11root = VALUE('X11ROOT',,env)
IF x11root = '' THEN DO
	SAY "The environment variable X11ROOT is not set. XFree86/OS2 won't run without it."
	EXIT
END
home = VALUE('HOME',,env)
IF home = '' THEN home = x11root

userresources = home'\resource.x11'
sysresources  = x11root'\XFree86\lib\X11\xinit\resource'

/* merge in defaults */

IF exists(sysresources) THEN
	xrdb -merge sysresources

IF exists(userresources) THEN
	xrdb -merge userresources

/* start some nice programs */
'start/min 'twm
'start/min 'xclock -geometry 50x50-1+1
/* 'start/min 'xterm -geometry 80x50+494+51 */
/* 'start/min 'xterm -geometry 80x20+494-0 */
xterm -geometry 80x66+0+0 -name login

exit

exists:
'DIR "'arg(1)'" >nul 2>&1'
if rc = 0 THEN RETURN 1
RETURN 0
