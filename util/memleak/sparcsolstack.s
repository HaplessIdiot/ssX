! $XFree86$
	.seg	"text"
	.proc	16
	.globl	getStackPointer
getStackPointer:
	retl
	mov	%sp,%o0
	.globl	getFramePointer
getFramePointer:
	retl
	mov	%fp,%o0
