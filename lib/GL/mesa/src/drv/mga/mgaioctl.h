/* $XFree86$ */

#ifndef MGA_IOCTL_H
#define MGA_IOCTL_H

#include "mgalib.h"
#include "mga_xmesa.h"

GLbitfield mgaClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		     GLint cx, GLint cy, GLint cw, GLint ch ); 


void mgaSwapBuffers( mgaContextPtr mmesa ); 



mgaUI32 *mgaAllocVertexDwords( mgaContextPtr mmesa, int dwords );


void mgaGetILoadBufferLocked( mgaContextPtr mmesa );


void mgaFireILoadLocked( mgaContextPtr mmesa, 
			 GLuint offset, GLuint length );

void mgaWaitAgeLocked( mgaContextPtr mmesa, int age );
void mgaWaitAge( mgaContextPtr mmesa, int age );
int mgaUpdateLock( mgaContextPtr mmesa, drmLockFlags flags );

void mgaFlushVertices( mgaContextPtr mmesa ); 
void mgaFlushVerticesLocked( mgaContextPtr mmesa );


void mgaFireEltsLocked( mgaContextPtr mmesa, 
			GLuint start, 
			GLuint end,
			GLuint discard );

void mgaGetEltBufLocked( mgaContextPtr mmesa );
void mgaReleaseBufLocked( mgaContextPtr mmesa, drmBufPtr buffer );

void mgaFlushEltsLocked( mgaContextPtr mmesa );
void mgaFlushElts( mgaContextPtr mmesa ) ;


/* upload texture
 */

void mgaDDFinish( GLcontext *ctx );

void mgaDDInitIoctlFuncs( GLcontext *ctx );
 
#define FLUSH_BATCH(mmesa) do {						\
        if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)  				\
              fprintf(stderr, "FLUSH_BATCH in %s\n", __FUNCTION__);	\
	if (mmesa->vertex_dma_buffer) mgaFlushVertices(mmesa);		\
	else if (mmesa->next_elt != mmesa->first_elt) mgaFlushElts(mmesa);	        	\
} while (0)

extern drmBufPtr mga_get_buffer_ioctl( mgaContextPtr mmesa );

static __inline
mgaUI32 *mgaAllocVertexDwordsInline( mgaContextPtr mmesa, int dwords )
{
   int bytes = dwords * 4;
   mgaUI32 *head;

   if (!mmesa->vertex_dma_buffer) {
      LOCK_HARDWARE( mmesa );

      if (mmesa->first_elt != mmesa->next_elt) 
	 mgaFlushEltsLocked(mmesa);

      mmesa->vertex_dma_buffer = mga_get_buffer_ioctl( mmesa );
      UNLOCK_HARDWARE( mmesa );
   } else if (mmesa->vertex_dma_buffer->used + bytes > 
	      mmesa->vertex_dma_buffer->total) {
      LOCK_HARDWARE( mmesa );
      mgaFlushVerticesLocked( mmesa );
      mmesa->vertex_dma_buffer = mga_get_buffer_ioctl( mmesa );
      UNLOCK_HARDWARE( mmesa );
   }

   head = (mgaUI32 *)((char *)mmesa->vertex_dma_buffer->address + 
		      mmesa->vertex_dma_buffer->used);

   mmesa->vertex_dma_buffer->used += bytes;
   return head;
}


#endif
