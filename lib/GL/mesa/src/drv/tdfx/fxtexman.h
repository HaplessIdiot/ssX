/* $XFree86: $ */
#ifndef FXTEXMAN_H
#define FXTEXMAN_H


#define fxTMGetTexInfo(o) ((tfxTexInfo*)((o)->DriverData))

#define fxTMMoveOutTM_NoLock fxTMMoveOutTM

extern void fxTMReloadMipMapLevel(GLcontext *, struct gl_texture_object *,
                                  GLint);
extern void fxTMReloadSubMipMapLevel(GLcontext *,
                                     struct gl_texture_object *, GLint, GLint,
                                     GLint);

extern void fxTMInit(fxMesaContext ctx);

extern void fxTMClose(fxMesaContext ctx);

extern void fxTMRestoreTextures_NoLock(fxMesaContext ctx);

extern void fxTMMoveInTM(fxMesaContext, struct gl_texture_object *, GLint);

extern void fxTMMoveOutTM(fxMesaContext, struct gl_texture_object *);

extern void fxTMFreeTexture(fxMesaContext, struct gl_texture_object *);


#endif
