# $XFree86:$




# $XConsortium: egc_asm.s /main/2 1995/12/29 11:49:16 kaleb $

	.text

#
#
#
	.globl	_getbits_x
_getbits_x:
	pushl	%esi
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax		#  x
	movl	24(%esp),%ebx		#  patternwidth
	movl	28(%esp),%esi		#  lineptr
	pushl	%esi
	movl	%eax,%edx
	movl	%eax,%ecx
	shrl	$3,%ecx
	addl	%ecx,%esi
	movb	%al,%cl
	movb	(%esi),%ah
	incl	%esi
	movb	(%esi),%al
	popl	%esi
	andb	$07,%cl
	shlw	%cl,%ax
	addw	$8,%dx
	subw	%bx,%dx
	js	getbits_0
	jz	getbits_0
	movb	%dl,%cl
	shrb	%cl,%ah
	movb	(%esi),%al
	shlw	%cl,%ax
getbits_0:
	cmpw	$8,%bx
	jl	getbits_xx
	movl	32(%esp),%ecx
	shrb	%cl,%ah
	movb	%ah,%al
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%esi
	ret
getbits_xx:
	movb	$8,%cl
	subb	%bl,%cl
	movb	$0xff,%al
	shlb	%cl,%al
	andb	%al,%ah
	cmpb	$1,%bl
	je	bit1
	cmpb	$2,%bl
	je	bit2
	cmpb	$4,%bl
	je	bit4
	jmp	bit3_x
bit1:                        
	movb	%ah,%al
	shrb	$1,%al
	orb	%al,%ah
bit2:
	movb	%ah,%al
	shrb	$2,%al
	orb	%al,%ah
bit4:
	movb	%ah,%al
	shrb	$4,%al
	orb	%al,%ah
	jmp	getbits_1
bit3_x:
	cmpb	$3,%bl
	je	bit3
	cmpb	$6,%bl
	je	bit6
	jmp	bit5
bit3:
	movb	%ah,%al
	shrb	$3,%al
	orb	%al,%ah
bit6:
	movb	%ah,%al
	shrb	$6,%al
	orb	%al,%ah
	jmp	getbits_1
bit5:
	cmpb	$5,%bl
	jne	bit7
	movb	%ah,%al
	shrb	$5,%al
	orb	%al,%ah
	jmp	getbits_1
bit7:
	movb	%ah,%al
	shrb	$7,%al
	orb	%al,%ah
getbits_1:
	movl	32(%esp),%ecx
	shrb	%cl,%ah
	movb	%ah,%al
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%esi
	ret
#
#
#
	.globl _wcopyr
_wcopyr:
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	movl	_vgaBase,%eax
	cmpl	%eax,%edi
	jge	wcopyr_0
	addl	$2,%edi
	movw	(%esi),%ax
	movw	%ax,(%edi)
	addl	$2,%esi
	decl	%ecx
wcopyr_0:
	cld
	rep
	movsw
	popl	%edi
	popl	%esi
	ret

	.globl	_wcopyl
_wcopyl:
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	20(%esp),%ecx
	movl	_vgaBase,%edi
	movl	%ecx,%edx
	decl	%edx
	shll	%edx
	subl	%edx,%esi
	xorw	%ax,%ax
	cmpl	%edi,%esi
	jge	wcopyl_0
	movb	$1,%al
	decl	%ecx
wcopyl_0:
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	std
	rep
	movsw
	testb	%al,%al
	jz	wcopyl_ret
	addl	$2,%esi
	movsw
wcopyl_ret:
	popl	%edi
	popl	%esi
	ret

	.globl	_read8Z
_read8Z:
	pushl	%esi
	pushl	%edi
	pushl	%ecx
	pushl	%ebx
	movl	20(%esp),%esi
	movw	$0x04a2,%dx
	movw	$0x43ff,%ax
	outw	%ax,%dx
	movb	$0x42,%ah
	movb	(%esi),%bl
	shll	$8,%ebx
	outw	%ax,%dx
	movb	$0x41,%ah
	movb	(%esi),%bl
	shll	$8,%ebx
	outw	%ax,%dx
	movb	$0x40,%ah
	movb	(%esi),%bl
	shll	$8,%ebx
	outw	%ax,%dx
	xorl	%eax,%eax
	movb	(%esi),%bl
	movl	$32,%ecx
read8Z_1:
	rcrl	$1,%ebx
	rcll	$4,%eax
	loop	read8Z_1

	popl	%ebx
	popl	%ecx
	popl	%edi
	popl	%esi
	ret
