#define SIS_TRI_FUNC 1

static void
SIS_TAG (sis_tri) (GLcontext * ctx, GLuint v0, GLuint v1, GLuint v2,
		   GLuint pv)
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  struct vertex_buffer *VB = ctx->VB;
  float *MMIOBase = (float *) GET_IOBase (hwcx);

  STEREO_SAMPLE(v0);
  
  /* specular color is considered */
  mWait3DCmdQueue (10 * 3 + 1);

  if (SIS_STATES & (SIS_SMOOTH))
    {
      /* 
       * not move it to UpdateState because maybe optimization can be done
       * according to FAN and STRIP
       */
      hwcx->dwPrimitiveSet &= ~0x07001f07;
      hwcx->dwPrimitiveSet |=
	(OP_3D_FIRE_TSARGBc | SHADE_GOURAUD | OP_3D_TRIANGLE_DRAW);      
    }
  else
    {
      hwcx->dwPrimitiveSet &= ~0x07001f07;
      hwcx->dwPrimitiveSet |=
	(OP_3D_FIRE_TSARGBc | SHADE_FLAT_VertexC | OP_3D_TRIANGLE_DRAW);
    }

  {
    ((DWORD *) MMIOBase)[REG_3D_PrimitiveSet / 4] = hwcx->dwPrimitiveSet;
  }
  
#define LAST_VERTEX 0
  SIS_MMIO_WRITE_VERTEX (v0, 0);
  SIS_MMIO_WRITE_VERTEX (v1, 1);
#undef LAST_VERTEX
#define LAST_VERTEX 1
  SIS_MMIO_WRITE_VERTEX (v2, 2);
#undef LAST_VERTEX

  /* debug
  mEndPrimitive ();
  WaitEngIdle(hwcx); 
  d2f_once (ctx);
  */
}

static void
SIS_TAG (sis_agp_tri) (GLcontext * ctx, GLuint v0, GLuint v1,
		       GLuint v2, GLuint pv)
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  struct vertex_buffer *VB = ctx->VB;

  DWORD dcSARGB;

  STEREO_SAMPLE(v0);

  /* delete if VB size <= AGP_ALLOC_SIZE */
#if 0
  if ((DWORD) AGP_CurrentPtr - (DWORD) AGP_StartPtr >= (AGP_ALLOC_SIZE - 0x10))
    {
      sis_FlushAGP (ctx);
      sis_StartAGP (ctx);
    }
#endif

#define FIRST_VERTEX 1
  SIS_AGP_WRITE_VERTEX (v0);
#undef FIRST_VERTEX
#define FIRST_VERTEX 0
  SIS_AGP_WRITE_VERTEX (v1);
  SIS_AGP_WRITE_VERTEX (v2);
#undef FIRST_VERTEX
}

#undef SIS_MMIO_WRITE_VERTEX
#undef SIS_AGP_WRITE_VERTEX

#undef SIS_TAG
#undef SIS_IS_SMOOTH
#undef SIS_STATES
#undef SIS_TRI_FUNC
