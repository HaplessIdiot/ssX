/* OS/2 rexx script to emulate the "cd dir; command" mechanism in make
 * which does not work with stupid CMD.EXE
 *
 * $XFree86$
 */
curdir = directory()
line = arg(1)
new = directory(word(line,1))
subword(line,2)
old = directory(curdir)
exit
