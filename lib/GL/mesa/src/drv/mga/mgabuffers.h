#ifndef MGA_BUFFERS_H
#define MGA_BUFFERS_H

GLboolean mgaDDSetReadBuffer(GLcontext *ctx, GLenum mode );
GLboolean mgaDDSetDrawBuffer(GLcontext *ctx, GLenum mode );

void mgaUpdateRects( mgaContextPtr mmesa, GLuint buffers );

#endif
