

/*
 * Try to do a fast glDrawPixels.  Conditions include:
 *   not using a display list
 *   simple pixel unpacking
 *   no raster ops
 *   etc....
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
GLboolean gl_direct_DrawPixels( GLcontext *ctx,
                                const struct gl_pixelstore_attrib *unpack,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GLubyte rgb[MAX_WIDTH][3];
   GLubyte rgba[MAX_WIDTH][4];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glDrawPixels", 
                                                  GL_FALSE);


   if (ctx->RenderMode != GL_RENDER)
      return GL_FALSE;

   if (!ctx->Current.RasterPosValid) {
      /* no-op */
      return GL_TRUE;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   /* see if device driver can do the drawpix */
   if (ctx->Driver.DrawPixels) {
      GLint x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      GLint y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      if ((*ctx->Driver.DrawPixels)(ctx, x, y, width, height, format, type,
                                    unpack, pixels))
         return GL_TRUE;
   }

   if ((ctx->RasterMask&(~(SCISSOR_BIT|WINCLIP_BIT)))==0
       && ctx->Pixel.RedBias==0.0 && ctx->Pixel.RedScale==1.0
       && ctx->Pixel.GreenBias==0.0 && ctx->Pixel.GreenScale==1.0
       && ctx->Pixel.BlueBias==0.0 && ctx->Pixel.BlueScale==1.0
       && ctx->Pixel.AlphaBias==0.0 && ctx->Pixel.AlphaScale==1.0
       && ctx->Pixel.IndexShift==0 && ctx->Pixel.IndexOffset==0
       && ctx->Pixel.MapColorFlag==0
       && unpack->Alignment==1
       && !unpack->SwapBytes
       && !unpack->LsbFirst) {

      GLint destX = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      GLint destY = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      GLint drawWidth = width;           /* actual width drawn */
      GLint drawHeight = height;         /* actual height drawn */
      GLint skipPixels = unpack->SkipPixels;
      GLint skipRows = unpack->SkipRows;
      GLint rowLength;
      GLdepth zSpan[MAX_WIDTH];  /* only used when zooming */
      GLint zoomY0 = 0;

      if (unpack->RowLength > 0)
         rowLength = unpack->RowLength;
      else
         rowLength = width;

      /* If we're not using pixel zoom then do all clipping calculations
       * now.  Otherwise, we'll let the gl_write_zoomed_*_span() functions
       * handle the clipping.
       */
      if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
         /* horizontal clipping */
         if (destX < ctx->Buffer->Xmin) {
            skipPixels += (ctx->Buffer->Xmin - destX);
            drawWidth  -= (ctx->Buffer->Xmin - destX);
            destX = ctx->Buffer->Xmin;
         }
         if (destX + drawWidth > ctx->Buffer->Xmax)
            drawWidth -= (destX + drawWidth - ctx->Buffer->Xmax - 1);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY < ctx->Buffer->Ymin) {
            skipRows   += (ctx->Buffer->Ymin - destY);
            drawHeight -= (ctx->Buffer->Ymin - destY);
            destY = ctx->Buffer->Ymin;
         }
         if (destY + drawHeight > ctx->Buffer->Ymax)
            drawHeight -= (destY + drawHeight - ctx->Buffer->Ymax - 1);
         if (drawHeight <= 0)
            return GL_TRUE;
      } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
         /* CUED MODIFICATION */
         /* horizontal clipping */
         if (destX < ctx->Buffer->Xmin) {
            skipPixels += (ctx->Buffer->Xmin - destX);
            drawWidth  -= (ctx->Buffer->Xmin - destX);
            destX = ctx->Buffer->Xmin;
         }
         if (destX + drawWidth > ctx->Buffer->Xmax)
            drawWidth -= (destX + drawWidth - ctx->Buffer->Xmax - 1);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY > ctx->Buffer->Ymax) {
            skipRows   += (destY - ctx->Buffer->Ymax - 1);
            drawHeight -= (destY - ctx->Buffer->Ymax - 1);
            destY = ctx->Buffer->Ymax + 1;
         }
         if (destY - drawHeight < ctx->Buffer->Ymin)
            drawHeight -= (ctx->Buffer->Ymin - (destY - drawHeight));
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else {
         /* setup array of fragment Z value to pass to zoom function */
         GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
         GLint i;
         assert(drawWidth < MAX_WIDTH);
         for (i=0; i<drawWidth; i++)
            zSpan[i] = z;

         /* save Y value of first row */
         zoomY0 = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      }


      /*
       * Ready to draw!
       * The window region at (destX, destY) of size (drawWidth, drawHeight)
       * will be written to.
       * We'll take pixel data from buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */

      if (format==GL_RGBA && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels) * 4;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (void *) src, NULL);
                  src += rowLength * 4;
                  destY++;
               }
            } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* CUED MODIFICATION */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY-1,
                                              (void *) src, NULL);
                  src += rowLength * 4;
                  destY--;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) src, zoomY0);
                  src += rowLength * 4;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_RGB && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels) * 3;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) src, NULL);
                  src += rowLength * 3;
                  destY++;
               }
            } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* CUED MODIFICATION */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY-1,
                                              (void *) src, NULL);
                  src += rowLength * 3;
                  destY--;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  gl_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                           zSpan, (void *) src, zoomY0);
                  src += rowLength * 3;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_LUMINANCE && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels);
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
                  }
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) rgb, NULL);
                  src += rowLength;
                  destY++;
               }
            } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* CUED MODIFICATION */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
                  }
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY-1,
                                              (void *) rgb, NULL);
                  src += rowLength;
                  destY--;
               }
            }
            else {
               /* with zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
                  }
                  gl_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                           zSpan, (void *) rgb, zoomY0);
                  src += rowLength;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_LUMINANCE_ALPHA && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels)*2;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLubyte *ptr = src;
                  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
                  }
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (void *) rgba, NULL);
                  src += rowLength*2;
                  destY++;
               }
            } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
              /* CUED MODIFICATION */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLubyte *ptr = src;
                  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
                  }
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY-1,
                                               (void *) rgba, NULL);
                  src += rowLength*2;
                  destY--;
               }
            }
            else {
               /* with zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLubyte *ptr = src;
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
                  }
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) rgba, zoomY0);
                  src += rowLength*2;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_COLOR_INDEX && type==GL_UNSIGNED_BYTE) {
         GLubyte *src = (GLubyte *) pixels + skipRows * rowLength + skipPixels;
         if (ctx->Visual->RGBAflag) {
            /* convert CI data to RGBA */
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  assert(drawWidth < MAX_WIDTH);
                  gl_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (const GLubyte (*)[4])rgba, 
                                               NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            } else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               GLint row;
               /* CUED MODIFICATION */
               for (row=0; row<drawHeight; row++) {
                  assert(drawWidth < MAX_WIDTH);
                  gl_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY-1,
                                               (const GLubyte (*)[4])rgba, 
                                               NULL);
                  src += rowLength;
                  destY--;
               }
               return GL_TRUE;
            }

            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  assert(drawWidth < MAX_WIDTH);
                  gl_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) rgba, zoomY0);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
         }
         else {
            /* write CI data to CI frame buffer */
            GLint row;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteCI8Span)(ctx, drawWidth, destX, destY,
                                              src, NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               return GL_FALSE;
            }
         }
      }
      else {
         /* can't handle this pixel format and/or data type here */
         return GL_FALSE;
      }
   }
   else {
      /* can't do direct render, have to use slow path */
      return GL_FALSE;
   }
}
