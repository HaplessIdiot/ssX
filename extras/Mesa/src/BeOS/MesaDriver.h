#ifndef MESA_DRIVER_H
#define MESA_DRIVER_H

#include "glheader.h"

#include <assert.h>
#include <stdio.h>

extern "C" {

#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

}	// extern "C"

// BeOS component ordering for B_RGBA32 bitmap format
#define BE_RCOMP 2
#define BE_GCOMP 1
#define BE_BCOMP 0
#define BE_ACOMP 3

#define PACK_B_RGBA32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | (color[ACOMP] << 24))

#define PACK_B_RGB32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | 0xFF000000)

#define FLIP(coord) (LIBGGI_MODE(ggi_ctx->ggi_visual)->visible.y-(coord) - 1) 

class BGLView;


//
// This object hangs off of the BGLView object.  We have to use
// Be's BGLView class as-is to maintain binary compatibility (we
// can't add new members to it).  Instead we just put all our data
// in this class and use BGLVIew::m_gc to point to it.
//
class MesaDriver
{
public:
	MesaDriver();
	~MesaDriver();
	void Init(BGLView * bglview, GLcontext * c, GLvisual * v, GLframebuffer * b);

	void LockGL();
	void UnlockGL();
	void SwapBuffers() const;
	void CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const;
	void Draw(BRect updateRect) const;

private:
	MesaDriver(const MesaDriver &rhs);  // copy constructor illegal
	MesaDriver &operator=(const MesaDriver &rhs);  // assignment oper. illegal

	GLcontext * 	m_glcontext;
	GLvisual * 		m_glvisual;
	GLframebuffer *	m_glframebuffer;

	BGLView *		m_bglview;
	BBitmap *		m_bitmap;

	GLubyte 		m_clear_color[4];  // buffer clear color
	GLuint 			m_clear_index;      // buffer clear color index
	GLint 			m_bottom;           // used for flipping Y coords
	GLuint 			m_width;
	GLuint			m_height;
	
   // Mesa Device Driver functions
   static void 		UpdateState(GLcontext *ctx, GLuint new_state);
   static void 		ClearIndex(GLcontext *ctx, GLuint index);
   static void 		ClearColor(GLcontext *ctx, const GLchan color[4]);
   static void 		Clear(GLcontext *ctx, GLbitfield mask,
                                GLboolean all, GLint x, GLint y,
                                GLint width, GLint height);
   static void 		ClearFront(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                          GLint width, GLint height);
   static void 		ClearBack(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                         GLint width, GLint height);
   static void 		Index(GLcontext *ctx, GLuint index);
   static void 		Color(GLcontext *ctx, GLubyte r, GLubyte g,
                     GLubyte b, GLubyte a);
   static void		SetDrawBuffer(GLcontext *ctx, GLenum mode);
   static void 		SetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode);
   static void 		GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                             GLuint *height);
   static const GLubyte *	GetString(GLcontext *ctx, GLenum name);

   // Front-buffer functions
   static void 		WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                      GLint x, GLint y,
                                      const GLchan color[4],
                                      const GLubyte mask[]);
   static void 		WriteRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    CONST GLubyte rgba[][4],
                                    const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const GLchan color[4],
                                        const GLubyte mask[]);
   static void 		WriteCI32SpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanFront(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);
   static void 		WriteCI32PixelsFront(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsFront(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanFront(const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 GLubyte rgba[][4]);
   static void 		ReadCI32PixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[]);

   // Back buffer functions
   static void 		WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[]);
   static void 		WriteRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[]);
   static void 		WriteCI32SpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanBack(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanBack(const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y, GLuint colorIndex,
                                   const GLubyte mask[]);
   static void 		WriteCI32PixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsBack(const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanBack(const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                GLubyte rgba[][4]);
   static void 		ReadCI32PixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[]);

};

#endif	// #ifndef MESA_DRIVER_H
