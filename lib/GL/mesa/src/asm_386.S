/* Id: asm_386.S,v 1.2 1999/02/26 08:52:30 martin Exp $ */

/*
 * asm-386.S - special (hopefully faster) transformation functions for x86
 *
 * by Josh Vanderhoof
 *
 * This file is in the public domain.
 */

/*
 * Log: asm_386.S,v $
 * Revision 1.2  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1998/11/01 20:20:29  brianp
 * updated with Josh's new x86 code
 *
 * Revision 3.2  1998/05/31 23:33:56  brianp
 * small change for OS/2 from Alexander Mai
 *
 * Revision 3.1  1998/03/05 03:08:32  brianp
 * added Josh's patches for rescale normal feature
 *
 * Revision 3.0  1998/01/31 20:45:56  brianp
 * initial rev
 *
 * Revision 1.8  1997/12/17 00:50:51  brianp
 * applied Josh's patch to fix texture coordinate transformation bugs
 *
 * Revision 1.7  1997/12/17 00:27:11  brianp
 * applied Josh's patch to fix bfris
 *
 * Revision 1.6  1997/12/01 01:02:41  brianp
 * added FreeBSD patches (Daniel J. O'Connor)
 *
 * Revision 1.5  1997/11/19 23:52:17  brianp
 * added missing "cld" instruction in asm_transform_points4_identity()
 *
 * Revision 1.4  1997/11/11 02:22:41  brianp
 * small change per Josh to ensure U/V pairing
 *
 * Revision 1.3  1997/11/07 03:37:24  brianp
 * added missing line from Stephane Rehel
 *
 * Revision 1.2  1997/11/07 03:30:37  brianp
 * added Josh's 11-5-97 patches
 *
 * Revision 1.1  1997/10/30 06:00:33  brianp
 * Initial revision
 */


#ifndef ALIGN
#define ALIGN 	.align 4, 0x90
#endif

#ifndef GLOBAL
#define GLOBAL(n) 	.globl n
#endif

/*
 * Change this to .data if your system doesn't have .rodata
 */
#ifndef RODATA
#if defined(__FreeBSD__) || defined(__EMX__)
#define RODATA .data
#else
#define RODATA 	.section .rodata
#endif
#endif

#ifndef DATA
#define DATA 	.data
#endif

#ifndef TEXT
#define TEXT 	.text
#endif

#if defined(FREEBSD)
#define asm_transform_points3_general _asm_transform_points3_general
#define asm_transform_points3_identity _asm_transform_points3_identity
#define asm_transform_points3_2d _asm_transform_points3_2d
#define asm_transform_points3_2d_no_rot _asm_transform_points3_2d_no_rot
#define asm_transform_points3_3d _asm_transform_points3_3d
#define asm_transform_points4_general _asm_transform_points4_general
#define asm_transform_points4_identity _asm_transform_points4_identity
#define asm_transform_points4_2d _asm_transform_points4_2d
#define asm_transform_points4_2d_no_rot _asm_transform_points4_2d_no_rot
#define asm_transform_points4_3d _asm_transform_points4_3d
#define asm_transform_points4_3d_no_rot _asm_transform_points4_3d_no_rot
#define asm_transform_points4_perspective _asm_transform_points4_perspective
#define asm_cliptest_points4 _asm_cliptest_points4
#define gl_transform_normals_3fv _gl_transform_normals_3fv
#endif

#define S(x) 	x * 4(%esi)
#define D(x) 	x * 4(%edi)
#define M(x, y) x * 16 + y * 4(%edx)

/*
 * void asm_transform_points*( GLmatrix *MATRIX, GLuint COUNT,
 *                             GLfloat SOURCE[][STRIDE], GLfloat DEST[][4],
 *                             GLuint STRIDE );
 */

#define PARAM(p) L_ARG_BASE + L_ARG_ ## p (%esp)
#define SETUP_PARAM L_ARG_BASE = 4
#define PUSHL(a) pushl a ; L_ARG_BASE = L_ARG_BASE+4
#define POPL(a) popl a ; L_ARG_BASE = L_ARG_BASE-4
L_ARG_MATRIX = 0
L_ARG_COUNT = 4
L_ARG_SOURCE = 8
L_ARG_DEST = 12
L_ARG_STRIDE = 16

GLOBAL(asm_transform_points3_general)
ALIGN
asm_transform_points3_general:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(STRIDE), %eax
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(0)
	fmuls M(0, 2)
	flds S(0)
	fmuls M(0, 3)

	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(1, 2)
	flds S(1)
	fmuls M(1, 3)

	/*
	 * The FPU stack should now look like this:
	 *
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(0) * M(0, 2)
	 * st(4) = S(0) * M(0, 3)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(1) * M(1, 2)
	 * st(0) = S(1) * M(1, 3)
	 */

	fxch %st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp %st, %st(7) 	/* 1 2 0 4 5 6 7 */
	fxch %st(1)		/* 2 1 0 4 5 6 7 */
	faddp %st, %st(5) 	/* 1 0 4 5 6 7 */
	faddp %st, %st(3) 	/* 0 4 5 6 7 */
	faddp %st, %st(1) 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 */

	flds S(2)
	fmuls M(2, 0)
	flds S(2)
	fmuls M(2, 1)
	flds S(2)
	fmuls M(2, 2)
	flds S(2)
	fmuls M(2, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 * st(3) = S(2) * M(2, 0)
	 * st(2) = S(2) * M(2, 1)
	 * st(1) = S(2) * M(2, 2)
	 * st(0) = S(2) * M(2, 3)
	 */

	fxch %st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp %st, %st(7) 	/* 1 2 0 4 5 6 7 */
	fxch %st(1)		/* 2 1 0 4 5 6 7 */
	faddp %st, %st(5) 	/* 1 0 4 5 6 7 */
	faddp %st, %st(3) 	/* 0 4 5 6 7 */
	faddp %st, %st(1) 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 */

	fxch %st(3) 	/* 3 1 2 0 */
	fadds M(3, 0)
	fxch %st(2) 	/* 2 1 3 0 */
	fadds M(3, 1)
	fxch %st(1) 	/* 1 2 3 0 */
	fadds M(3, 2)
	fxch %st(3) 	/* 0 2 3 1 */
	fadds M(3, 3)

	/*
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + M(3, 2)
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + M(3, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + M(3, 1)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3) + M(3, 3)
	 */

	fxch %st(3) 	/* 3 1 2 0 */
	fstps D(2) 	/* 1 2 0 */
	fxch %st(1) 	/* 2 1 0 */
	fstps D(0) 	/* 1 0 */
	leal (%esi, %eax, 4), %esi
	fstps D(1) 	/* 0 */
	decl %ecx
	fstps D(3) 	/* */

	leal D(4), %edi

	jnz 1b

2:	POPL(%edi)
	POPL(%esi)
	ret


GLOBAL(asm_transform_points3_identity)
ALIGN
asm_transform_points3_identity:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebx)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	movl S(0), %eax
	movl S(1), %edx
	movl S(2), %ebx
	leal (%esi, %ebp, 4), %esi
	movl %eax, D(0)
	movl %edx, D(1)
	movl %ebx, D(2)
	movl $0x3f800000, D(3)
	decl %ecx
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%ebx)
	POPL(%edi)
	POPL(%esi)
	ret


GLOBAL(asm_transform_points3_2d)
ALIGN
asm_transform_points3_2d:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testb $1, %cl
	jz 1f

	decl %ecx

	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)

	/*
	 * st(3) = S(0) * M(0, 0)
	 * st(2) = S(0) * M(0, 1)
	 * st(1) = S(1) * M(1, 0)
	 * st(0) = S(1) * M(1, 1)
	 */

	fxch %st(1) 	/* 1 0 2 3 */
	fadds M(3, 0)
	fxch %st(1) 	/* 0 1 2 3 */
	fadds M(3, 1)
	fxch %st(1) 	/* 1 0 2 3 */
	faddp %st(3) 	/* 0 2 3 */
	faddp %st(1) 	/* 2 3 */
	fstps D(1) 	/* 3 */
	fstps D(0) 	/* */
	movl S(2), %eax
	leal (%esi, %ebp, 4), %esi
	movl $0x3f800000, D(3)
	movl %eax, D(2)
	leal D(4), %edi

1:	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(4)
	fmuls M(0, 0)
	flds S(4)
	fmuls M(0, 1)
	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(5)
	fmuls M(1, 0)
	flds S(5)
	fmuls M(1, 1)

	/*
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(4) * M(0, 0)
	 * st(4) = S(4) * M(0, 1)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(5) * M(1, 0)
	 * st(0) = S(5) * M(1, 1)
	 */

	fxch %st(7) 	/* 7 1 2 3 4 5 6 0 */
	fadds M(3, 0)
	fxch %st(6) 	/* 6 1 2 3 4 5 7 0 */
	fadds M(3, 1)
	fxch %st(5) 	/* 5 1 2 3 4 6 7 0 */
	fadds M(3, 0)
	fxch %st(4) 	/* 4 1 2 3 5 6 7 0 */
	fadds M(3, 1)

	movl S(2), %eax
	movl $0x3f800000, D(3)
	movl %eax, D(2)
	movl S(6), %eax
	movl $0x3f800000, D(7)
	movl %eax, D(6)
	leal (%esi, %ebp, 8), %esi
	subl $2, %ecx

	/*
	 * st(7) = S(5) * M(1, 1)
	 * st(6) = S(0) * M(0, 0) + M(3, 0)
	 * st(5) = S(0) * M(0, 1) + M(3, 1)
	 * st(4) = S(4) * M(0, 0) + M(3, 0)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(5) * M(1, 0)
	 * st(0) = S(4) * M(0, 1) + M(3, 1)
	 */

	faddp %st(7) 	/* 1 2 3 4 5 6 7 */
	faddp %st(3) 	/* 2 3 4 5 6 7 */
	faddp %st(3) 	/* 3 4 5 6 7 */
	faddp %st(3) 	/* 4 5 6 7 */
	fxch %st(3) 	/* 7 5 6 4 */
	fstps D(5) 	/* 5 6 4 */
	fstps D(1) 	/* 6 4 */
	fstps D(0) 	/* 4 */
	fstps D(4) 	/* */

	leal D(8), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%edi)
	POPL(%esi)
	ret


GLOBAL(asm_transform_points3_2d_no_rot)
ALIGN
asm_transform_points3_2d_no_rot:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

	leal (, %ebp, 4), %ebp

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(1)
	fmuls M(1, 1)
	fxch %st(1)
	fadds M(3, 0)
	fxch %st(1)
	fadds M(3, 1)
	fxch %st(1)
	fstps D(0)
	fstps D(1)

	movl S(2), %eax
	movl $0x3f800000, D(3)
	movl %eax, D(2)

	decl %ecx
	leal (%esi, %ebp, 4), %esi
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points3_3d)
ALIGN
asm_transform_points3_3d:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(STRIDE), %eax
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(0)
	fmuls M(0, 2)

	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(1, 2)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(0) * M(0, 2)
	 * st(2) = S(1) * M(1, 0)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(1, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 */

	flds S(2)
	fmuls M(2, 0)
	flds S(2)
	fmuls M(2, 1)
	flds S(2)
	fmuls M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(2) = S(2) * M(2, 0)
	 * st(1) = S(2) * M(2, 1)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 */

	fxch %st(2) 	/* 2 1 0 */
	fadds M(3, 0)
	fxch %st(1) 	/* 1 2 0 */
	fadds M(3, 1)
	fxch %st(2) 	/* 0 2 1 */
	fadds M(3, 2)

	fxch %st(1) 	/* 2 0 1 */
	fstps D(0) 	/* 0 1 */
	fstps D(2) 	/* 1 */
	fstps D(1) 	/* */
	movl $0x3f800000, D(3)

	leal (%esi, %eax, 4), %esi
	decl %ecx

	leal D(4), %edi

	jnz 1b

2:	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points4_general)
ALIGN
asm_transform_points4_general:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(STRIDE), %eax
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(0)
	fmuls M(0, 2)
	flds S(0)
	fmuls M(0, 3)

	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(1, 2)
	flds S(1)
	fmuls M(1, 3)

	/*
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(0) * M(0, 2)
	 * st(4) = S(0) * M(0, 3)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(1) * M(1, 2)
	 * st(0) = S(1) * M(1, 3)
	 */

	fxch %st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp %st, %st(7) 	/* 1 2 0 4 5 6 7 */
	fxch %st(1)		/* 2 1 0 4 5 6 7 */
	faddp %st, %st(5) 	/* 1 0 4 5 6 7 */
	faddp %st, %st(3) 	/* 0 4 5 6 7 */
	faddp %st, %st(1) 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 */

	flds S(2)
	fmuls M(2, 0)
	flds S(2)
	fmuls M(2, 1)
	flds S(2)
	fmuls M(2, 2)
	flds S(2)
	fmuls M(2, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 * st(3) = S(2) * M(2, 0)
	 * st(2) = S(2) * M(2, 1)
	 * st(1) = S(2) * M(2, 2)
	 * st(0) = S(2) * M(2, 3)
	 */

	fxch %st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp %st, %st(7) 	/* 1 2 0 4 5 6 7 */
	fxch %st(1)		/* 2 1 0 4 5 6 7 */
	faddp %st, %st(5) 	/* 1 0 4 5 6 7 */
	faddp %st, %st(3) 	/* 0 4 5 6 7 */
	faddp %st, %st(1) 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 */

	flds S(3)
	fmuls M(3, 0)
	flds S(3)
	fmuls M(3, 1)
	flds S(3)
	fmuls M(3, 2)
	flds S(3)
	fmuls M(3, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 * st(3) = S(3) * M(3, 0)
	 * st(2) = S(3) * M(3, 1)
	 * st(1) = S(3) * M(3, 2)
	 * st(0) = S(3) * M(3, 3)
	 */

	fxch %st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp %st, %st(7) 	/* 1 2 0 4 5 6 7 */
	fxch %st(1)		/* 2 1 0 4 5 6 7 */
	faddp %st, %st(5) 	/* 1 0 4 5 6 7 */
	faddp %st, %st(3) 	/* 0 4 5 6 7 */

	leal (%esi, %eax, 4), %esi
	decl %ecx

	faddp %st, %st(1) 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + S(3) * M(3, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + S(3) * M(3, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + S(3) * M(3, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3) + S(3) * M(3, 3)
	 */

	fxch %st(3) 	/* 3 1 2 0 */
	fstps D(0) 	/* 1 2 0 */
	fxch %st(1) 	/* 2 1 0 */
	fstps D(1) 	/* 1 0 */
	fstps D(2) 	/* 0 */
	fstps D(3) 	/* */

	leal D(4), %edi

	jnz 1b

2:	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points4_identity)
ALIGN
asm_transform_points4_identity:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

1:	movl S(0), %eax
	movl S(1), %edx
	movl %eax, D(0)
	movl %edx, D(1)
	movl S(2), %eax
	movl S(3), %edx
	decl %ecx
	leal (%esi, %ebp, 4), %esi
	movl %eax, D(2)
	movl %edx, D(3)
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points4_2d)
ALIGN
asm_transform_points4_2d:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebx)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(3)
	fmuls M(3, 0)
	flds S(3)
	fmuls M(3, 1)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(3) * M(3, 0)
	 * st(0) = S(3) * M(3, 1)
	 */

	movl S(2), %eax
	movl S(3), %ebx
	leal (%esi, %ebp, 4), %esi
	decl %ecx
	movl %eax, D(2)
	movl %ebx, D(3)
	faddp %st(4)
	faddp %st(4)
	faddp %st(2)
	faddp %st(2)
	fstps D(1)
	fstps D(0)
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%ebx)
	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points4_2d_no_rot)
ALIGN
asm_transform_points4_2d_no_rot:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebx)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(3)
	fmuls M(3, 0)
	flds S(3)
	fmuls M(3, 1)
	movl S(2), %eax
	movl S(3), %ebx
	leal (%esi, %ebp, 4), %esi
	decl %ecx
	movl %eax, D(2)
	movl %ebx, D(3)
	faddp %st(2)
	faddp %st(2)
	fstps D(1)
	fstps D(0)
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%ebx)
	POPL(%edi)
	POPL(%esi)
	ret



GLOBAL(asm_transform_points4_3d)
ALIGN
asm_transform_points4_3d:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(STRIDE), %eax
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(3)

	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(0, 1)
	flds S(0)
	fmuls M(0, 2)

	flds S(1)
	fmuls M(1, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(1, 2)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(0) * M(0, 2)
	 * st(2) = S(1) * M(1, 0)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(1, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 */

	flds S(2)
	fmuls M(2, 0)
	flds S(2)
	fmuls M(2, 1)
	flds S(2)
	fmuls M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(2) = S(2) * M(2, 0)
	 * st(1) = S(2) * M(2, 1)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 */

	flds S(3)
	fmuls M(3, 0)
	flds S(3)
	fmuls M(3, 1)
	flds S(3)
	fmuls M(3, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(2) = S(3) * M(3, 0)
	 * st(1) = S(3) * M(3, 1)
	 * st(0) = S(3) * M(3, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */

	leal (%esi, %eax, 4), %esi
	decl %ecx

	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + S(3) * M(3, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + S(3) * M(3, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + S(3) * M(3, 2)
	 */

	fxch %st(2) 	/* 2 1 0 */
	fstps D(0) 	/* 1 0 */
	fstps D(1) 	/* 0 */
	fstps D(2) 	/* */
	fstps D(3)

	leal D(4), %edi

	jnz 1b

2:	POPL(%edi)
	POPL(%esi)
	ret

GLOBAL(asm_transform_points4_3d_no_rot)
ALIGN
asm_transform_points4_3d_no_rot:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(2)
	fmuls M(2, 2)

	flds S(3)
	fmuls M(3, 0)
	flds S(3)
	fmuls M(3, 1)
	flds S(3)
	fmuls M(3, 2)

	movl S(3), %eax
	leal (%esi, %ebp, 4), %esi
	decl %ecx
	movl %eax, D(3)

	faddp %st(3)
	faddp %st(3)
	faddp %st(3)

	fstps D(2)
	fstps D(1)
	fstps D(0)

	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%edi)
	POPL(%esi)
	ret

GLOBAL(asm_transform_points4_perspective)
ALIGN
asm_transform_points4_perspective:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	PUSHL(%ebp)
	movl PARAM(STRIDE), %ebp
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

ALIGN
1:	flds S(0)
	fmuls M(0, 0)
	flds S(1)
	fmuls M(1, 1)
	flds S(2)
	fmuls M(2, 2)

	flds S(2)
	fmuls M(2, 0)
	flds S(2)
	fmuls M(2, 1)
	flds S(3)
	fmuls M(3, 2)

	movl S(2), %eax
	leal (%esi, %ebp, 4), %esi
	xorl $0x80000000, %eax
	decl %ecx

	faddp %st(3)
	faddp %st(3)
	faddp %st(3)

	fstps D(2)
	fstps D(1)
	fstps D(0)

	movl %eax, D(3)
	leal D(4), %edi
	jnz 1b

2:	POPL(%ebp)
	POPL(%edi)
	POPL(%esi)
	ret



/*
 * Table for clip test.
 *
 * 	bit6 = S(3) < 0
 * 	bit5 = S(2) < 0
 * 	bit4 = abs(S(2)) > abs(S(3))
 * 	bit3 = S(1) < 0
 * 	bit2 = abs(S(1)) > abs(S(3))
 * 	bit1 = S(0) < 0
 * 	bit0 = abs(S(0)) > abs(S(3))
 */

/* Vertex buffer clipping flags (from vb.h) */
#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_ALL_BITS    0x3f

#define MAGN_X(i) 	(~(((i) & 1) - 1))
#define SIGN_X(i) 	(~((((i) >> 1) & 1) - 1))
#define MAGN_Y(i) 	(~((((i) >> 2) & 1) - 1))
#define SIGN_Y(i) 	(~((((i) >> 3) & 1) - 1))
#define MAGN_Z(i) 	(~((((i) >> 4) & 1) - 1))
#define SIGN_Z(i) 	(~((((i) >> 5) & 1) - 1))
#define SIGN_W(i) 	(~((((i) >> 6) & 1) - 1))

#define CLIP_VALUE(i) 						\
	 (CLIP_RIGHT_BIT 					\
	  & ((~SIGN_X(i) & SIGN_W(i)) 				\
	     | (~SIGN_X(i) & ~SIGN_W(i) & MAGN_X(i)) 		\
	     | (SIGN_X(i) & SIGN_W(i) & ~MAGN_X(i)))) 		\
	 | (CLIP_LEFT_BIT 					\
	    & ((SIGN_X(i) & SIGN_W(i)) 				\
	       | (~SIGN_X(i) & SIGN_W(i) & ~MAGN_X(i)) 		\
	       | (SIGN_X(i) & ~SIGN_W(i) & MAGN_X(i)))) 	\
	 | (CLIP_TOP_BIT 					\
	    & ((~SIGN_Y(i) & SIGN_W(i)) 			\
	       | (~SIGN_Y(i) & ~SIGN_W(i) & MAGN_Y(i)) 		\
	       | (SIGN_Y(i) & SIGN_W(i) & ~MAGN_Y(i)))) 	\
	 | (CLIP_BOTTOM_BIT 					\
	    & ((SIGN_Y(i) & SIGN_W(i)) 				\
	       | (~SIGN_Y(i) & SIGN_W(i) & ~MAGN_Y(i)) 		\
	       | (SIGN_Y(i) & ~SIGN_W(i) & MAGN_Y(i)))) 	\
	 | (CLIP_FAR_BIT 					\
	    & ((~SIGN_Z(i) & SIGN_W(i)) 			\
	       | (~SIGN_Z(i) & ~SIGN_W(i) & MAGN_Z(i)) 		\
	       | (SIGN_Z(i) & SIGN_W(i) & ~MAGN_Z(i)))) 	\
	 | (CLIP_NEAR_BIT 					\
	    & ((SIGN_Z(i) & SIGN_W(i)) 				\
	       | (~SIGN_Z(i) & SIGN_W(i) & ~MAGN_Z(i)) 		\
	       | (SIGN_Z(i) & ~SIGN_W(i) & MAGN_Z(i))))

#define CLIP_VALUE8(i) \
	CLIP_VALUE(i + 0), CLIP_VALUE(i + 1), CLIP_VALUE(i + 2), CLIP_VALUE(i + 3), \
	CLIP_VALUE(i + 4), CLIP_VALUE(i + 5), CLIP_VALUE(i + 6), CLIP_VALUE(i + 7)

RODATA

clip_table:
	.byte CLIP_VALUE8(0x00)
	.byte CLIP_VALUE8(0x08)
	.byte CLIP_VALUE8(0x10)
	.byte CLIP_VALUE8(0x18)
	.byte CLIP_VALUE8(0x20)
	.byte CLIP_VALUE8(0x28)
	.byte CLIP_VALUE8(0x30)
	.byte CLIP_VALUE8(0x38)
	.byte CLIP_VALUE8(0x40)
	.byte CLIP_VALUE8(0x48)
	.byte CLIP_VALUE8(0x50)
	.byte CLIP_VALUE8(0x58)
	.byte CLIP_VALUE8(0x60)
	.byte CLIP_VALUE8(0x68)
	.byte CLIP_VALUE8(0x70)
	.byte CLIP_VALUE8(0x78)

TEXT

/*
 * void asm_cliptest_points*( GLuint n, 
 *                            CONST GLfloat vClip[][4], 
 *                            GLubyte clipMask[],
 *                            GLubyte *orMask, 
 *                            GLubyte *andMask );
 */

L_ARG_COUNT = 0
L_ARG_SOURCE = 4
L_ARG_DEST = 8
L_ARG_ORMASK = 12
L_ARG_ANDMASK = 16

GLOBAL(asm_cliptest_points4)
ALIGN
asm_cliptest_points4:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(ORMASK), %eax
	movl PARAM(ANDMASK), %edx
	movl PARAM(COUNT), %ecx
	movl PARAM(SOURCE), %esi
	movl PARAM(DEST), %edi

	movb (%eax), %al
	movb (%edx), %ah

	call cliptest

	movl PARAM(ORMASK), %ecx
	movl PARAM(ANDMASK), %edx

	movb %al, (%ecx)
	movb %ah, (%edx)

	POPL(%edi)
	POPL(%esi)
	ret

/*
 * cliptest -
 *
 * inputs:
 * 	ecx = # points
 * 	esi = points
 * 	edi = clipmask[]
 *
 * inputs/outputs:
 * 	al = ormask
 * 	ah = andmask
 */

cliptest:
	testl %ecx, %ecx
	jz 2f

	pushl %ebp
	pushl %ebx

#if defined(__ELF__) && defined(__PIC__)
	/* store pointer to clip_table on stack */
	call 3f
	addl $_GLOBAL_OFFSET_TABLE_, %ebx
	movl clip_table@GOT(%ebx), %ebx
	pushl %ebx
	jmp 1f

3:	/* store eip in ebx */
	movl (%esp), %ebx
	ret
#endif

ALIGN
1:	movl S(3), %ebp
	movl S(2), %ebx

	xorl %edx, %edx
	addl %ebp, %ebp	/* %ebp = abs(S(3))*2 ; carry = sign of S(3) */

	adcl %edx, %edx
	addl %ebx, %ebx	/* %ebx = abs(S(2))*2 ; carry = sign of S(2) */

	adcl %edx, %edx
	cmpl %ebx, %ebp	/* carry = abs(S(2))*2 > abs(S(3))*2 */

	adcl %edx, %edx
	movl S(1), %ebx

	addl %ebx, %ebx	/* %ebx = abs(S(1))*2 ; carry = sign of S(1) */

	adcl %edx, %edx
	cmpl %ebx, %ebp	/* carry = abs(S(1))*2 > abs(S(3))*2 */

	adcl %edx, %edx
	movl S(0), %ebx

	addl %ebx, %ebx	/* %ebx = abs(S(0))*2 ; carry = sign of S(0) */

	adcl %edx, %edx
	cmpl %ebx, %ebp	/* carry = abs(S(0))*2 > abs(S(3))*2 */

	adcl %edx, %edx

#if defined(__ELF__) && defined(__PIC__)
	movl (%esp), %ebp

	leal S(4), %esi
	movb (%edi), %bl

	movb (%ebp, %edx, 1), %dl
#else
	leal S(4), %esi

	movb (%edi), %bl
	movb clip_table(%edx), %dl
#endif

	orb %dl, %bl
	orb %dl, %al

	andb %dl, %ah
	movb %bl, (%edi)

	incl %edi
	decl %ecx

	jnz 1b

#if defined(__ELF__) && defined(__PIC__)
	addl $4, %esp
#endif
	popl %ebx
	popl %ebp
2:	ret


/*
 * unsigned int inverse_nofp( float f );
 *
 * Calculate the inverse of a float without using the FPU.
 * This function returns a float in eax, so it's return
 * type should be 'int' when called from C (and converted
 * to float with pointer/union abuse).
 */
ALIGN
inverse_nofp:

	/* get mantissa in eax */
	movl 4(%esp), %ecx
	andl $0x7fffff, %ecx

	/* set implicit integer */
	orl $0x800000, %ecx

	/* div 0x10000:0x00000000 by mantissa */
	xorl %eax, %eax
	movl $0x10000, %edx

	divl %ecx
	
	/* round result */
	shrl $1, %eax
	adcl $0, %eax

	/* get exponent in ecx */
	movl $0x7f800000, %ecx
	movl 4(%esp), %edx
	andl %edx, %ecx

	/* negate exponent and decrement it */
	movl $253 << 23, %edx
	subl %ecx, %edx

	/* if bit 24 is set, shift and adjust exponent */
	testl $0x1000000, %eax
	jz 1f

	shrl $1, %eax
	addl $1 << 23, %edx

	/* combine mantissa and exponent, then set sign */
1:	andl $0x7fffff, %eax
	movl 4(%esp), %ecx
	orl %edx, %eax
	andl $0x80000000, %ecx
	orl %ecx, %eax

	ret


/*
 * void gl_transform_normals_3fv( GLuint n, 
 *                                const GLfloat *in,
 *                                GLuint in_stride,
 *                                const GLfloat m[16],
 *                                GLfloat out[][3], 
 *                                GLboolean normalize,
 *                                GLboolean rescale)
 * 
 */

L_ARG_COUNT = 0
L_ARG_SOURCE = 4
L_ARG_STRIDE = 8
L_ARG_MATRIX = 12
L_ARG_DEST = 16
L_ARG_NORMALIZE = 20
L_ARG_RESCALE = 24

GLOBAL(gl_transform_normals_3fv)
ALIGN
gl_transform_normals_3fv:
	SETUP_PARAM
	PUSHL(%esi)
	PUSHL(%edi)
	movl PARAM(STRIDE), %eax
	movl PARAM(COUNT), %ecx
	movl PARAM(DEST), %edi
	movl PARAM(MATRIX), %edx
	movl PARAM(SOURCE), %esi

	testl %ecx, %ecx
	jz 2f

	/*
	 * Check if rescale is needed
	 */
	cmpl $0, PARAM(RESCALE)
	jz 3f

	/*
	 * Transform and rescale
	 */
	flds M(0, 2)
	fmuls M(0, 2)
	flds M(1, 2)
	fmuls M(1, 2)
	flds M(2, 2)
	fmuls M(2, 2)
	fxch
	faddp %st(2)
	faddp %st(1)
	fsqrt
	fld1
	fdivp %st, %st(1)

1:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(1, 0)
	flds S(0)
	fmuls M(2, 0)

	flds S(1)
	fmuls M(0, 1)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(2, 1)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(1, 0)
	 * st(3) = S(0) * M(2, 0)
	 * st(2) = S(1) * M(0, 1)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(2, 1)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 */

	flds S(2)
	fmuls M(0, 2)
	flds S(2)
	fmuls M(1, 2)
	flds S(2)
	fmuls M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(4) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 * st(2) = S(2) * M(0, 2)
	 * st(1) = S(2) * M(1, 2)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1) + S(2) * M(0, 2)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1) + S(2) * M(1, 2)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1) + S(2) * M(2, 2)
	 */

	fld %st(3)
	fmul %st, %st(1)
	fmul %st, %st(2)
	fmulp %st(3)

	fstps D(2)
	fstps D(1)
	fstps D(0)

	leal (%esi, %eax, 4), %esi
	decl %ecx

	leal D(3), %edi

	jnz 1b

	fstp %st(0)

	jmp 4f

	/*
	 * Transform (no rescale)
	 */
ALIGN
3:	flds S(0)
	fmuls M(0, 0)
	flds S(0)
	fmuls M(1, 0)
	flds S(0)
	fmuls M(2, 0)

	flds S(1)
	fmuls M(0, 1)
	flds S(1)
	fmuls M(1, 1)
	flds S(1)
	fmuls M(2, 1)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(1, 0)
	 * st(3) = S(0) * M(2, 0)
	 * st(2) = S(1) * M(0, 1)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(2, 1)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 */

	flds S(2)
	fmuls M(0, 2)
	flds S(2)
	fmuls M(1, 2)
	flds S(2)
	fmuls M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(4) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 * st(2) = S(2) * M(0, 2)
	 * st(1) = S(2) * M(1, 2)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch %st(2) 		/* 2 1 0 3 4 5 */
	faddp %st, %st(5) 	/* 1 0 3 4 5 */
	faddp %st, %st(3) 	/* 0 3 4 5 */
	faddp %st, %st(1) 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1) + S(2) * M(0, 2)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1) + S(2) * M(1, 2)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1) + S(2) * M(2, 2)
	 */

	fxch %st(2) 	/* 2 1 0 */
	fstps D(0) 	/* 1 0 */
	fstps D(1) 	/* 0 */
	fstps D(2) 	/* */

	leal (%esi, %eax, 4), %esi
	decl %ecx

	leal D(3), %edi

	jnz 3b

	/*
	 * Skip normalize if it isn't needed
	 */
4:	cmpl $0, PARAM(NORMALIZE)
	jz 2f

	/* Normalize required */

	movl PARAM(COUNT), %esi
	movl PARAM(DEST), %edi

	subl $4, %esp 	/* temp var for 1.0 / len */

	/*
	 * (%esp) = length of first normal
	 */
	flds D(0)
	fmuls D(0)
	flds D(1)
	fmuls D(1)
	flds D(2)
	fmuls D(2)
	fxch %st(2)
	faddp %st(1)
	faddp %st(1)
	fsqrt
	fstps (%esp)

	jmp 3f

ALIGN
1:	/* %st(0) = length of next normal */
	flds D(3)
	fmuls D(3)
	flds D(4)
	fmuls D(4)
	flds D(5)
	fmuls D(5)
	fxch %st(2)
	faddp %st(1)
	faddp %st(1)
	fsqrt

	/*
	 * inverse the length of the current normal, which is
	 * already at (%esp).  This should overlap the prev
	 * fsqrt nicely.
	 */
	call inverse_nofp
	movl %eax, (%esp)

	/* multiply normal by 1/len */
	flds D(0)
	fmuls (%esp)
	flds D(1)
	fmuls (%esp)
	flds D(2)
	fmuls (%esp)
	fxch %st(3)
	fstps (%esp) 	/* store length of next normal */
	fstps D(1)
	fstps D(0)
	fstps D(2)
	leal D(3), %edi
3:	decl %esi
	jnz 1b

	/* finish up the last normal */
	call inverse_nofp
	movl %eax, (%esp)
	flds D(0)
	fmuls (%esp)
	flds D(1)
	fmuls (%esp)
	flds D(2)
	fmuls (%esp)
	fxch %st(2)
	fstps D(0)
	fstps D(1)
	fstps D(2)

	addl $4, %esp

2:	POPL(%edi)
	POPL(%esi)
	ret

/* end */
