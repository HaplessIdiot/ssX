#ifndef MGA_IOCTL_H
#define MGA_IOCTL_H

#include "i810context.h"


GLuint *i810AllocDwords( i810ContextPtr imesa, int dwords, GLuint prim );

void i810GetGeneralDmaBufferLocked( i810ContextPtr mmesa ); 

void i810FlushVertices( i810ContextPtr mmesa ); 
void i810FlushVerticesLocked( i810ContextPtr mmesa );

void i810FlushGeneralLocked( i810ContextPtr imesa );
void i810WaitAgeLocked( i810ContextPtr imesa, int age );
void i810WaitAge( i810ContextPtr imesa, int age );

void i810DmaFinish( i810ContextPtr imesa );

void i810RegetLockQuiescent( i810ContextPtr imesa );

void i810DDInitIoctlFuncs( GLcontext *ctx );

void i810SwapBuffers( i810ContextPtr imesa );

GLbitfield i810Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		      GLint cx, GLint cy, GLint cw, GLint ch );

#define FLUSH_BATCH(imesa) do {						\
        if (I810_DEBUG&DEBUG_VERBOSE_IOCTL)  				\
              fprintf(stderr, "FLUSH_BATCH in %s\n", __FUNCTION__);	\
	if (imesa->vertex_dma_buffer) i810FlushVertices(imesa);		\
} while (0)


#endif
