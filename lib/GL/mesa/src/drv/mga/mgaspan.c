#include "types.h"
#include "mgadd.h"
#include "mgalib.h"
#include "mgadma.h"
#include "mgalog.h"
#include "mgaspan.h"

#define DBG 0


#define LOCAL_VARS					\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;	\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;	\
   GLuint pitch = mgaScreen->frontPitch;		\
   GLuint height = dPriv->h;				\
   char *read_buf = (char *)(sPriv->pFB +		\
			mmesa->readOffset +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch);              \
   char *buf = (char *)(sPriv->pFB +			\
			mmesa->drawOffset +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch);              \
   GLushort p = MGA_CONTEXT( ctx )->MonoColor;          \
   (void) read_buf; (void) buf; (void) p
   


#define LOCAL_DEPTH_VARS				\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;	\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;	\
   GLuint pitch = mgaScreen->frontPitch;		\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(sPriv->pFB +			\
			mgaScreen->depthOffset +	\
			dPriv->x * 2 +			\
			dPriv->y * pitch)

#define INIT_MONO_PIXEL(p) 

#define CLIPPIXEL(_x,_y) (_x >= minx && _x < maxx && \
			  _y >= miny && _y < maxy)


#define CLIPSPAN(_x,_y,_n,_x1,_n1,_i)				\
	 if (_y < miny || _y >= maxy) _n1 = 0, _x1 = x;		\
         else {							\
            _n1 = _n;						\
	    _x1 = _x;						\
	    if (_x1 < minx) _i += (minx - _x1), _x1 = minx;	\
	    if (_x1 + _n1 >= maxx) n1 -= (_x1 + n1 - maxx) + 1;	\
         }



#define HW_LOCK()				\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);	\
   LOCK_HARDWARE_QUIESCENT(mmesa);

#define HW_CLIPLOOP()						\
  do {								\
    int _nc = mmesa->numClipRects;				\
    while (_nc--) {						\
       int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;	\
       int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY;	\
       int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;	\
       int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;

#define HW_ENDCLIPLOOP()			\
    }						\
  } while (0)

#define HW_UNLOCK()				\
    UNLOCK_HARDWARE(mmesa);







/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define Y_FLIP(_y) (height - _y - 1)
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 8) & 0xf8;					\
   rgba[1] = (p >> 3) & 0xfc;					\
   rgba[2] = (p << 3) & 0xf8;					\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) mga##x##_565
#include "spantmp.h"




/* 15 bit, 555 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )			\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = (((r & 0xf8) << 7) |	\
		                            ((g & 0xf8) << 3) |	\
                         		    ((b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 7) & 0xf8;					\
   rgba[1] = (p >> 3) & 0xf8;					\
   rgba[2] = (p << 3) & 0xf8;					\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) mga##x##_555
#include "spantmp.h"



/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + _x*2 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + _x*2 + _y*pitch);	

#define TAG(x) mga##x##_16
#include "depthtmp.h"




void mgaDDInitSpanFuncs( GLcontext *ctx )
{
   if (1) {
      ctx->Driver.WriteRGBASpan = mgaWriteRGBASpan_565;
      ctx->Driver.WriteRGBSpan = mgaWriteRGBSpan_565;
      ctx->Driver.WriteMonoRGBASpan = mgaWriteMonoRGBASpan_565;
      ctx->Driver.WriteRGBAPixels = mgaWriteRGBAPixels_565;
      ctx->Driver.WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_565;
      ctx->Driver.ReadRGBASpan = mgaReadRGBASpan_565;
      ctx->Driver.ReadRGBAPixels = mgaReadRGBAPixels_565;
   } else {
      ctx->Driver.WriteRGBASpan = mgaWriteRGBASpan_555;
      ctx->Driver.WriteRGBSpan = mgaWriteRGBSpan_555;
      ctx->Driver.WriteMonoRGBASpan = mgaWriteMonoRGBASpan_555;
      ctx->Driver.WriteRGBAPixels = mgaWriteRGBAPixels_555;
      ctx->Driver.WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_555;
      ctx->Driver.ReadRGBASpan = mgaReadRGBASpan_555;
      ctx->Driver.ReadRGBAPixels = mgaReadRGBAPixels_555;
   }

   ctx->Driver.ReadDepthSpan = mgaReadDepthSpan_16;
   ctx->Driver.WriteDepthSpan = mgaWriteDepthSpan_16;
   ctx->Driver.ReadDepthPixels = mgaReadDepthPixels_16;
   ctx->Driver.WriteDepthPixels = mgaWriteDepthPixels_16;

   ctx->Driver.WriteCI8Span        =NULL;
   ctx->Driver.WriteCI32Span       =NULL;
   ctx->Driver.WriteMonoCISpan     =NULL;
   ctx->Driver.WriteCI32Pixels     =NULL;
   ctx->Driver.WriteMonoCIPixels   =NULL;
   ctx->Driver.ReadCI32Span        =NULL;
   ctx->Driver.ReadCI32Pixels      =NULL;
}
