static void SIS_TAG (sis_WriteDepthSpan) (GLcontext * ctx, GLuint n, GLint x,
					  GLint y, const GLdepth depth[],
					  const GLubyte mask[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base = SIS_SW_Z_BASE (x, Y_FLIP (y));

  int i;
  for (i = 0; i < n; i++, base++)
    {
      if (mask[i])
	SIS_SW_I2D (depth[i], *base);
    }
}

static void SIS_TAG (sis_ReadDepthSpan) (GLcontext * ctx, GLuint n, GLint x,
					 GLint y, GLdepth depth[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base = SIS_SW_Z_BASE (x, Y_FLIP (y));

  int i;
  for (i = 0; i < n; i++, base++)
    {
      SIS_SW_D2I (*base, depth[i]);
    }
}

static void SIS_TAG (sis_WriteDepthPixels) (GLcontext * ctx, GLuint n,
					    const GLint x[], const GLint y[],
					    const GLdepth depth[],
					    const GLubyte mask[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base;

  int i;
  for (i = 0; i < n; i++, base++)
    {
      if (mask[i])
	{
	  base = SIS_SW_Z_BASE (x[i], Y_FLIP (y[i]));
	  SIS_SW_I2D (depth[i], *base);
	}
    }
}

static void SIS_TAG (sis_ReadDepthPixels) (GLcontext * ctx, GLuint n,
					   const GLint x[], const GLint y[],
					   GLdepth depth[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base;

  int i;
  for (i = 0; i < n; i++, base++)
    {
      base = SIS_SW_Z_BASE (x[i], Y_FLIP (y[i]));
      SIS_SW_D2I (*base, depth[i]);
    }
}

#ifdef SIS_SW_STENCIL_FUNC

static void SIS_TAG (sis_WriteStencilSpan) (GLcontext * ctx, GLuint n,
					    GLint x, GLint y,
					    const GLstencil depth[],
					    const GLubyte mask[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base = SIS_SW_Z_BASE (x, Y_FLIP (y));

  int i;
  for (i = 0; i < n; i++, base++)
    {
      if (mask[i])
	SIS_SW_I2S (depth[i], *base);
    }
}

static void SIS_TAG (sis_ReadStencilSpan) (GLcontext * ctx, GLuint n, GLint x,
					   GLint y, GLstencil depth[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base = SIS_SW_Z_BASE (x, Y_FLIP (y));

  int i;
  for (i = 0; i < n; i++, base++)
    {
      SIS_SW_S2I (*base, depth[i]);
    }
}

static void SIS_TAG (sis_WriteStencilPixels) (GLcontext * ctx, GLuint n,
					      const GLint x[],
					      const GLint y[],
					      const GLstencil depth[],
					      const GLubyte mask[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base;

  int i;
  for (i = 0; i < n; i++, base++)
    {
      if (mask[i])
	{
	  base = SIS_SW_Z_BASE (x[i], Y_FLIP (y[i]));
	  SIS_SW_I2S (depth[i], *base);
	}
    }
}

static void SIS_TAG (sis_ReadStencilPixels) (GLcontext * ctx, GLuint n,
					     const GLint x[], const GLint y[],
					     GLstencil depth[])
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  SIS_SW_DTYPE *base;

  int i;
  for (i = 0; i < n; i++, base++)
    {
      base = SIS_SW_Z_BASE (x[i], Y_FLIP (y[i]));
      SIS_SW_S2I (*base, depth[i]);
    }
}

# undef SIS_SW_S2I
# undef SIS_SW_I2S

#endif

#undef SIS_TAG
#undef SIS_SW_DTYPE
#undef SIS_SW_D2I
#undef SIS_SW_I2D
