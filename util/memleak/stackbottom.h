/* $XFree86$ */

typedef char *ptr_t;	/* A generic pointer to which we can add        */
			/* byte displacements.                          */
			/* Preferably identical to caddr_t, if it       */
			/* exists.                                      */

extern ptr_t GC_get_stack_base(void);

