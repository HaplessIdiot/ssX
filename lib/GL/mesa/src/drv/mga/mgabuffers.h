/* $XFree86$ */

#ifndef MGA_BUFFERS_H
#define MGA_BUFFERS_H

void mgaDDSetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode );
GLboolean mgaDDSetDrawBuffer(GLcontext *ctx, GLenum mode );

void mgaUpdateRects( mgaContextPtr mmesa, GLuint buffers );

#endif
