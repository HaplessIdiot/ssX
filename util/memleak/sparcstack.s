# $XFree86$
	.seg	"text"
	.proc	16
	.globl	_getStackPointer
_getStackPointer:
	retl
	mov	%sp,%o0
	.globl	_getFramePointer
_getFramePointer:
	retl
	mov	%fp,%o0
