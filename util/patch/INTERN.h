/* $Header: /vol1/history/xf86/xc/util/patch/Attic/INTERN.h,v 1.1 1994/04/27 07:36:21 dawes Exp $
 *
 * $Log: INTERN.h,v $
 * Revision 1.1  1994/04/27 07:36:21  dawes
 * Initial revision
 *
 * Revision 2.0  86/09/17  15:35:58  lwall
 * Baseline for netwide release.
 * 
 */

#ifdef EXT
#undef EXT
#endif
#define EXT

#ifdef INIT
#undef INIT
#endif
#define INIT(x) = x

#define DOINIT
