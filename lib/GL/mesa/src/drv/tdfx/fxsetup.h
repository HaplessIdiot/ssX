/* $XFree86: $ */
#ifndef FXSETUP_H
#define FXSETUP_H


extern void fxDDEnable(GLcontext *, GLenum, GLboolean);

extern void fxDDAlphaFunc(GLcontext *, GLenum, GLclampf);

extern void fxDDBlendFunc(GLcontext *, GLenum, GLenum);

extern void fxDDBlendFuncSeparate(GLcontext *ctx,
                                  GLenum sfactorRGB, GLenum sfactorA,
                                  GLenum dfactorRGB, GLenum dfactorA);

extern GrStencil_t fxConvertGLStencilOp(GLenum op);

extern void fxDDScissor(GLcontext *ctx,
                        GLint x, GLint y, GLsizei w, GLsizei h);

extern void fxDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat * params);

extern GLboolean fxDDColorMask(GLcontext *ctx,
                               GLboolean r, GLboolean g,
                               GLboolean b, GLboolean a);

extern void fxDDShadeModel(GLcontext * ctx, GLenum mode);

extern void fxDDCullFace(GLcontext * ctx, GLenum mode);

extern void fxDDFrontFace(GLcontext * ctx, GLenum mode);

extern void fxSetupFXUnits(GLcontext *);

#endif
