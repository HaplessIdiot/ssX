#ifndef DBG
#define DBG 0
#endif



static void TAG(WriteDepthSpan)( GLcontext *ctx,
				 GLuint n, GLint x, GLint y,
				 const GLdepth *depth, 
				 const GLubyte mask[] )
{
   HW_LOCK()
      {
	 GLint x1;
	 GLint n1;
	 LOCAL_DEPTH_VARS;

	 y = Y_FLIP(y);

	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);

	       if (DBG) fprintf(stderr, "WriteDepthSpan %d..%d (x1 %d)\n",
				(int)i, (int)n1, (int)x1);

	       if (mask)
	       {
		  for (;i<n1;i++,x1++)
		     if (mask[i])
			WRITE_DEPTH( x1, y, depth[i] );
	       }
	       else
	       {
		  for (;i<n1;i++,x1++)
		     WRITE_DEPTH( x1, y, depth[i] );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_UNLOCK();
}


static void TAG(WriteDepthPixels)( GLcontext *ctx,
				   GLuint n, 
				   const GLint x[], 
				   const GLint y[],
				   const GLdepth depth[], 
				   const GLubyte mask[] )
{
   HW_LOCK()
      {
	 GLint i;
	 LOCAL_DEPTH_VARS;

	 if (DBG) fprintf(stderr, "WriteDepthPixels\n");

	 HW_CLIPLOOP()
	    {
	       for (i=0;i<n;i++)
	       {
		  if (mask[i]) {
		     const int fy = Y_FLIP(y[i]);
		     if (CLIPPIXEL(x[i],fy))
			WRITE_DEPTH( x[i], fy, depth[i] );
		  }
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_UNLOCK();
}




/* Read depth spans and pixels
 */
static void TAG(ReadDepthSpan)( GLcontext *ctx,
				GLuint n, GLint x, GLint y,
				GLdepth depth[])
{
   HW_LOCK()
      {
	 GLint x1,n1;
	 LOCAL_DEPTH_VARS;

	 y = Y_FLIP(y);

	 if (DBG) fprintf(stderr, "ReadDepthSpan\n");

	 HW_CLIPLOOP() 
	    {
	       GLint i = 0;
	       CLIPSPAN(x,y,n,x1,n1,i);
	       for (;i<n1;i++)
		  READ_DEPTH( depth[i], (x1+i), y );
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_UNLOCK();
}

static void TAG(ReadDepthPixels)( GLcontext *ctx, GLuint n, 
				  const GLint x[], const GLint y[],
				  GLdepth depth[] )
{
   HW_LOCK()
      {
	 GLint i;
	 LOCAL_DEPTH_VARS;

	 if (DBG) fprintf(stderr, "ReadDepthPixels\n");
 
	 HW_CLIPLOOP()
	    {
	       for (i=0;i<n;i++) {
		  int fy = Y_FLIP( y[i] );
		  if (CLIPPIXEL( x[i], fy ))
		     READ_DEPTH( depth[i], x[i], fy );
	       }
	    }
	 HW_ENDCLIPLOOP();
      }
   HW_UNLOCK();
}


#undef WRITE_DEPTH
#undef READ_DEPTH
#undef TAG
