/* -*- mode: C; c-basic-offset:8 -*- */
/*
 * GLX Hardware Device Driver for Matrox Millenium G200
 * Copyright (C) 1999 Wittawat Yamwong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    original by Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 *	9/20/99 rewrite by John Carmack <johnc@idsoftware.com>
 *      13/1/00 port to DRI by Keith Whitwell <keithw@precisioninsight.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatex.c,v 1.3 2000/06/21 12:11:00 tsi Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "mm.h"
#include "mgalib.h"
#include "mgadma.h"
#include "mgatex.h"
#include "mgalog.h"
#include "mgaregs.h"

#include "simple_list.h"


/*
 * mgaDestroyTexObj
 * Free all memory associated with a texture and NULL any pointers
 * to it.
 */
static void mgaDestroyTexObj( mgaContextPtr mmesa, mgaTextureObjectPtr t ) {

 	if ( !t ) return;
  	  	
	/* free the texture memory */
	if (t->MemBlock) {
		mmFreeMem( t->MemBlock );
		t->MemBlock = 0;

		if (t->age > mmesa->dirtyAge)
			mmesa->dirtyAge = t->age;
	}
 
 	/* free mesa's link */   
	if (t->tObj) 
		t->tObj->DriverData = NULL;

	/* see if it was the driver's current object */
	if (t->bound)
		mmesa->CurrentTexObj[t->bound - 1] = 0; 
	
	remove_from_list(t);
	free( t );
}

static void mgaSwapOutTexObj(mgaContextPtr mmesa, mgaTextureObjectPtr t)
{
	if (t->MemBlock) {
		mmFreeMem(t->MemBlock);
		t->MemBlock = 0;      

		if (t->age > mmesa->dirtyAge)
			mmesa->dirtyAge = t->age;
	}

	t->dirty_images = ~0;
	move_to_tail(&(mmesa->SwappedOut), t);
}


static void mgaPrintLocalLRU( mgaContextPtr mmesa, int heap ) 
{
   mgaTextureObjectPtr t;
   int sz = 1 << (mmesa->mgaScreen->logTextureGranularity[heap]);

   fprintf(stderr, "\nLocal LRU, heap %d:\n", heap);

   foreach( t, &(mmesa->TexObjList[heap]) ) {
      if (!t->tObj)
	 fprintf(stderr, "Placeholder %d at %x sz %x\n", 
		 t->MemBlock->ofs / sz,
		 t->MemBlock->ofs,
		 t->MemBlock->size);      
      else
	 fprintf(stderr, "Texture (bound %d) at %x sz %x\n", 
		 t->bound,
		 t->MemBlock->ofs,
		 t->MemBlock->size);      

   }

   fprintf(stderr, "\n\n");
}

static void mgaPrintGlobalLRU( mgaContextPtr mmesa, int heap )
{
   int i, j;
   drm_mga_tex_region_t *list = mmesa->sarea->texList[heap];

   fprintf(stderr, "\nGlobal LRU, heap %d list %p:\n", heap, list);

   for (i = 0, j = MGA_NR_TEX_REGIONS ; i < MGA_NR_TEX_REGIONS ; i++) {
      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
	      j, list[j].age, list[j].next, list[j].prev);
      j = list[j].next;
      if (j == MGA_NR_TEX_REGIONS) break;
   }
   
   if (j != MGA_NR_TEX_REGIONS) {
      fprintf(stderr, "Loop detected in global LRU\n\n\n");
      for (i = 0 ; i < MGA_NR_TEX_REGIONS ; i++) {
	      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
		      i, list[i].age, list[i].next, list[i].prev);
      }
   }
   
   fprintf(stderr, "\n\n");
}


static void mgaResetGlobalLRU( mgaContextPtr mmesa, GLuint heap )
{
   drm_mga_tex_region_t *list = mmesa->sarea->texList[heap];
   int sz = 1 << mmesa->mgaScreen->logTextureGranularity[heap];
   int i;

   mmesa->texAge[heap] = ++mmesa->sarea->texAge[heap];

   if (0) fprintf(stderr, "mgaResetGlobalLRU %d\n", (int)heap);

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (MGA_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.  
    */
   for (i = 0 ; (i+1) * sz <= mmesa->mgaScreen->textureSize[heap] ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = mmesa->sarea->texAge[heap];   
   }

   i--;
   list[0].prev = MGA_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = MGA_NR_TEX_REGIONS;
   list[MGA_NR_TEX_REGIONS].prev = i;
   list[MGA_NR_TEX_REGIONS].next = 0;

}


static void mgaUpdateTexLRU( mgaContextPtr mmesa, mgaTextureObjectPtr t ) 
{
   int i;
   int heap = t->heap;
   int logsz = mmesa->mgaScreen->logTextureGranularity[heap];
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   drm_mga_tex_region_t *list = mmesa->sarea->texList[heap];
   
   mmesa->texAge[heap] = ++mmesa->sarea->texAge[heap];

   if (!t->MemBlock) {
	   fprintf(stderr, "no memblock\n\n");
	   return;
   }

   /* Update our local LRU
    */
   move_to_head( &(mmesa->TexObjList[heap]), t );


   if (0)
	   fprintf(stderr, "mgaUpdateTexLRU heap %d list %p\n", heap, list);


   /* Update the global LRU
    */
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = mmesa->texAge[heap];

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;
      
      /* insert_at_head(list, i)
       */
      list[i].prev = MGA_NR_TEX_REGIONS;
      list[i].next = list[MGA_NR_TEX_REGIONS].next;
      list[(unsigned)list[MGA_NR_TEX_REGIONS].next].prev = i;
      list[MGA_NR_TEX_REGIONS].next = i;
   }

   if (0) {
	   mgaPrintGlobalLRU(mmesa, t->heap);
	   mgaPrintLocalLRU(mmesa, t->heap);
   }
}

/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent 
 * the other client's textures.  
 */
static void mgaTexturesGone( mgaContextPtr mmesa,
			     GLuint heap,
			     GLuint offset, 
			     GLuint size,
			     GLuint in_use ) 
{
   mgaTextureObjectPtr t, tmp;



   foreach_s ( t, tmp, &(mmesa->TexObjList[heap]) ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
	 continue;




      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
      if (t->bound) 
	 mgaSwapOutTexObj( mmesa, t );
      else
	 mgaDestroyTexObj( mmesa, t );
   }

   
   if (in_use) {
      t = (mgaTextureObjectPtr) calloc(1,sizeof(*t));
      if (!t) return;

      t->heap = heap;
      t->MemBlock = mmAllocMem( mmesa->texHeap[heap], size, 0, offset);      
      if (!t->MemBlock) {
	      fprintf(stderr, "Couldn't alloc placeholder sz %x ofs %x\n",
		      (int)size, (int)offset);
	      mmDumpMemInfo( mmesa->texHeap[heap]);
	      return;
      }
      insert_at_head( &(mmesa->TexObjList[heap]), t );
   }
}


void mgaAgeTextures( mgaContextPtr mmesa, int heap )
{
	drm_mga_sarea_t *sarea = mmesa->sarea;
	int sz = 1 << (mmesa->mgaScreen->logTextureGranularity[heap]);
	int idx, nr = 0;

	/* Have to go right round from the back to ensure stuff ends up
	 * LRU in our local list...  Fix with a cursor pointer.
	 */
	for (idx = sarea->texList[heap][MGA_NR_TEX_REGIONS].prev ; 
	     idx != MGA_NR_TEX_REGIONS && nr < MGA_NR_TEX_REGIONS ; 
	     idx = sarea->texList[heap][idx].prev, nr++)
	{
		if (sarea->texList[heap][idx].age > mmesa->texAge[heap]) {
			mgaTexturesGone(mmesa, heap, idx * sz, sz, 1);
		}
	}

	if (nr == MGA_NR_TEX_REGIONS) {
		mgaTexturesGone(mmesa, heap, 0,
				mmesa->mgaScreen->textureSize[heap], 0);
		mgaResetGlobalLRU( mmesa, heap );
	}

	
	if (0) {
		mgaPrintGlobalLRU( mmesa, heap );
		mgaPrintLocalLRU( mmesa, heap );
	}

	mmesa->texAge[heap] = sarea->texAge[heap];
	mmesa->dirty |= MGA_UPLOAD_TEX0IMAGE | MGA_UPLOAD_TEX1IMAGE;
}


/*
 * mgaSetTexWrappings
 */
static void mgaSetTexWrapping( mgaTextureObjectPtr t,
			       GLenum sWrap, GLenum tWrap ) {
	if (sWrap == GL_REPEAT) {
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_clampu_enable;
	} else {
		t->Setup[MGA_TEXREG_CTL] |= TMC_clampu_enable;
	}
	if (tWrap == GL_REPEAT) {
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_clampv_enable;
	} else {
		t->Setup[MGA_TEXREG_CTL] |= TMC_clampv_enable;
	}
}

/*
 * mgaSetTexFilter
 */
static void mgaSetTexFilter(mgaTextureObjectPtr t, GLenum minf, GLenum magf) {
	switch (minf) {
	case GL_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_nrst);
		break;
	case GL_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_bilin);
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm1s);
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm4s);
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm2s);
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm8s);
		break;
	default:
		mgaError("mgaSetTexFilter(): not supported min. filter %d\n",
			 (int)minf);
		break;
	}

	switch (magf) {
	case GL_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_magfilter_MASK,TF_magfilter_nrst);
		break;
	case GL_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_magfilter_MASK,TF_magfilter_bilin);
		break;
	default:
		mgaError("mgaSetTexFilter(): not supported mag. filter %d\n",
			 (int)magf);
		break;
	}
  
	/* See OpenGL 1.2 specification */
	if (magf == GL_LINEAR && (minf == GL_NEAREST_MIPMAP_NEAREST || 
                            minf == GL_NEAREST_MIPMAP_LINEAR)) {
		/* c = 0.5 */
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],TF_fthres_MASK,
		  0x20 << TF_fthres_SHIFT);
	} else {
		/* c = 0 */
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],TF_fthres_MASK,
		  0x10 << TF_fthres_SHIFT);
	}

}

/*
 * mgaSetTexBorderColor
 */
static void mgaSetTexBorderColor(mgaTextureObjectPtr t, GLubyte color[4]) {
	t->Setup[MGA_TEXREG_BORDERCOL] = 
		MGAPACKCOLOR8888(color[0],color[1],color[2],color[3]);

}







/*
 * mgaUploadSubImageLocked
 *
 * Perform an iload based update of a resident buffer.  This is used for
 * both initial loading of the entire image, and texSubImage updates.
 *
 * Performed with the hardware lock held.
 */
static void mgaUploadSubImageLocked( mgaContextPtr mmesa,
				     mgaTextureObjectPtr t,
				     int level,	     
				     int x, int y, int width, int height ) {
	int		x2;
	int		dwords;
        int		offset;
	struct gl_texture_image *image;
	int		texelBytes, texelsPerDword, texelMaccess, length;

	if ( level < 0 || level >= MGA_TEX_MAXLEVELS ) {
		mgaMsg( 1, "mgaUploadSubImage: bad level: %i\n", level );
 	 	return;
	}

	image = t->tObj->Image[level];
	if ( !image ) {
		mgaError( "mgaUploadSubImage: NULL image\n" );
		return;
	}
	
	/* find the proper destination offset for this level */
   	offset = (t->MemBlock->ofs + 
		  t->offsets[level]);

	texelBytes = t->texelBytes;	
	switch( texelBytes ) {
	case 1:
		texelsPerDword = 4;
		texelMaccess = 0;
		break;
	case 2:
		texelsPerDword = 2;
		texelMaccess = 1;
		break;
	case 4:
		texelsPerDword = 1;
		texelMaccess = 2;
		break;
	default:
		return;
	}
			
		
	/* We can't do a subimage update if pitch is < 32 texels due
	 * to hardware XY addressing limits, so we will need to
	 * linearly upload all modified rows.
	 */
	if ( image->Width < 32 ) {
		x = 0;
		width = image->Width * height;
		height = 1;

		/* Assume that 1x1 textures aren't going to cause a
		 * bus error if we read up to four texels from that
		 * location: 
		 */
		if ( width < texelsPerDword ) {
			width = texelsPerDword;
		}
	} else {
		/* pad the size out to dwords.  The image is a pointer
		   to the entire image, so we can safely reference
		   outside the x,y,width,height bounds if we need to */
		x2 = x + width;
		x2 = (x2 + (texelsPerDword-1)) & ~(texelsPerDword-1);	
		x = (x + (texelsPerDword-1)) & ~(texelsPerDword-1);
		width = x2 - x;
	}

	/* we may not be able to upload the entire texture in one
	   batch due to register limits or dma buffer limits.
	   Recursively split it up. */
	while ( 1 ) {
		dwords = height * width / texelsPerDword;
		if ( dwords * 4 <= MGA_DMA_BUF_SZ ) {
			break;
		}
		mgaMsg(10, "mgaUploadSubImage: recursively subdividing\n" );

		mgaUploadSubImageLocked( mmesa, t, level, x, y, 
					 width, height >> 1 );
		y += ( height >> 1 );
		height -= ( height >> 1 );
	}
	
	mgaMsg(10, "mgaUploadSubImage: %i,%i of %i,%i at %i,%i\n", 
	       width, height, image->Width, image->Height, x, y );

	/* bump the performance counter */
	mgaglx.c_textureSwaps += ( dwords << 2 );
	
   	length = dwords * 4;

	/* Fill in the secondary buffer with properly converted texels
	 * from the mesa buffer. */
   	if(t->heap == MGA_CARD_HEAP) {
	   mgaGetILoadBufferLocked( mmesa );
	   mgaConvertTexture( (mgaUI32 *)mmesa->iload_buffer->address, 
			     texelBytes, image, x, y, width, height ); 
	   if(length < 64) length = 64;
	   mgaMsg(10, "TexelBytes : %d, offset: %d, length : %d\n",
		  texelBytes,
		  mmesa->mgaScreen->textureOffset[t->heap] +
		  offset + 
		  y * width * 4/texelsPerDword,
		  length);
	   
	   mgaFireILoadLocked( mmesa, 
			      mmesa->mgaScreen->textureOffset[t->heap] +
			      offset + 
			      y * width * 4/texelsPerDword,
			      length);
	} else {
 	/* This works, is slower for uploads to card space and needs
	 * additional synchronization with the dma stream.
	 */
	   mgaConvertTexture( (mgaUI32 *)
			      (mmesa->mgaScreen->texVirtual[t->heap] + 
					  offset + 
					  y * width * 4/texelsPerDword), 
			     texelBytes, image, x, y, width, height ); 
	}
}


static void mgaUploadTexLevel( mgaContextPtr mmesa, 
				     mgaTextureObjectPtr t,
				     int l )
{
	mgaUploadSubImageLocked( mmesa,
				 t,
				 l,
				 0, 0,
				 t->tObj->Image[l]->Width,
				 t->tObj->Image[l]->Height);
}



/*
 * mgaCreateTexObj
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */
static void mgaCreateTexObj(mgaContextPtr mmesa, struct gl_texture_object *tObj) 
{
	mgaTextureObjectPtr		t;
	int					i, ofs, size;
	struct gl_texture_image *image;
	int					LastLevel;
	int					s, s2;
	int					textureFormat;
	
	mgaMsg( 10,"mgaCreateTexObj( %p )\n", tObj );

	t = malloc( sizeof( *t ) );
	if ( !t ) {
		mgaError( "mgaCreateTexObj: Failed to malloc mgaTextureObject\n" );
		return;
	}
	memset( t, 0, sizeof( *t ) );
	
	image = tObj->Image[ 0 ];
	if ( !image ) {
		return;
	}

	if ( 0 ) {
		/* G400 texture format options */

	} else {
		/* G200 texture format options */
		
		switch( image->Format ) {
		case GL_RGB:
	 	case GL_LUMINANCE:
	 		if ( image->IntFormat != GL_RGB5 &&
			     ( image->IntFormat == GL_RGB8 ||
			       mgaglx.default32BitTextures ) ) {
	 			t->texelBytes = 4;
	 			textureFormat = TMC_tformat_tw32;
	 		} else {
				t->texelBytes = 2;
				textureFormat = TMC_tformat_tw16;
			}
			break;
		case GL_ALPHA:
	 	case GL_LUMINANCE_ALPHA:
		case GL_INTENSITY:
		case GL_RGBA:
	 		if ( image->IntFormat != GL_RGBA4 &&
			     ( image->IntFormat == GL_RGBA8 ||
			       mgaglx.default32BitTextures ) ) {
	 			t->texelBytes = 4;
	 			textureFormat = TMC_tformat_tw32;
	 		} else {
				t->texelBytes = 2;
				textureFormat = TMC_tformat_tw12;
			}
			break;
		case GL_COLOR_INDEX:
			textureFormat = TMC_tformat_tw8;
			t->texelBytes = 1;
			break;
		default:
			mgaError( "mgaCreateTexObj: bad image->Format\n" );
			free( t );
			return;	
		}
	}
		
	/* we are going to upload all levels that are present, even if
	   later levels wouldn't be used by the current filtering mode.  This
	   allows the filtering mode to change without forcing another upload
	   of the images */
	LastLevel = MGA_TEX_MAXLEVELS-1;

	ofs = 0;
	for ( i = 0 ; i <= LastLevel ; i++ ) {
		int levelWidth, levelHeight;
		
		t->offsets[i] = ofs;
		image = tObj->Image[ i ];
		if ( !image ) {
			LastLevel = i - 1;
			mgaMsg( 10, "  missing images after LastLevel: %i\n",
				LastLevel );
			break;
		}
		/* the G400 doesn't work with textures less than 8
                   units in size */
		levelWidth = image->Width;
		levelHeight = image->Height;
		if ( levelWidth < 8 ) {
			levelWidth = 8;
		}
		if ( levelHeight < 8 ) {
			levelHeight = 8;
		}
		size = levelWidth * levelHeight * t->texelBytes;
		size = ( size + 31 ) & ~31;	/* 32 byte aligned */
		ofs += size;
		t->dirty_images |= (1<<i);
	}
	t->totalSize = ofs;
	t->lastLevel = LastLevel;
	

	/* fill in our mga texture object */
	t->tObj = tObj;
	t->ctx = mmesa;
	t->age = 0;
	t->bound = 0;

	
	insert_at_tail(&(mmesa->SwappedOut), t);
	
	t->MemBlock = 0;

	/* base image */	  
	image = tObj->Image[ 0 ];

	/* setup hardware register values */		
	t->Setup[MGA_TEXREG_CTL] = (TMC_takey_1 | 
				       TMC_tamask_0 | 
				       textureFormat );

	if (image->WidthLog2 >= 3) {
		t->Setup[MGA_TEXREG_CTL] |= ((image->WidthLog2 - 3) << 
						TMC_tpitch_SHIFT);
	} else {
		t->Setup[MGA_TEXREG_CTL] |= (TMC_tpitchlin_enable | 
						(image->Width << 
						 TMC_tpitchext_SHIFT));
	}


	t->Setup[MGA_TEXREG_CTL2] = TMC_ckstransdis_enable;

	if ( mmesa->glCtx->Light.Model.ColorControl == 
	     GL_SEPARATE_SPECULAR_COLOR ) 
	{
		t->Setup[MGA_TEXREG_CTL2] |= TMC_specen_enable;
	}
	
	t->Setup[MGA_TEXREG_FILTER] = (TF_minfilter_nrst | 
					  TF_magfilter_nrst |
					  TF_filteralpha_enable | 
					  (0x10 << TF_fthres_SHIFT) | 
					  (LastLevel << TF_mapnb_SHIFT));
	
	/* warp texture registers */
	if (MGA_IS_G200(mmesa)) {
		ofs = 28;
	} else {
   		ofs = 11;
   	}

	s = image->Width;
	s2 = image->WidthLog2;
	t->Setup[MGA_TEXREG_WIDTH] = 
		MGA_FIELD(TW_twmask, s - 1) |
		MGA_FIELD(TW_rfw,    (10 - s2 - 8) & 63 ) |
		MGA_FIELD(TW_tw,     (s2 + ofs ) | 0x40 );

	
	s = image->Height;
	s2 = image->HeightLog2;
	t->Setup[MGA_TEXREG_HEIGHT] = 
		MGA_FIELD(TH_thmask, s - 1) |
		MGA_FIELD(TH_rfh,    (10 - s2 - 8) & 63 ) | 
		MGA_FIELD(TH_th,     (s2 + ofs ) | 0x40 );


	/* set all the register values for filtering, border, etc */	
	mgaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
	mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
	mgaSetTexBorderColor( t, tObj->BorderColor );

  	tObj->DriverData = t;
}

static void mgaMigrateTexture( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
	/* NOT DONE */
}


static int mgaChooseTexHeap( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
	return 0;
}


int mgaUploadTexImages( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
	int heap;
	int i;
	int ofs;
	mgaglx.c_textureSwaps++;

	heap = t->heap = mgaChooseTexHeap( mmesa, t );

	/* Do we need to eject LRU texture objects?
	 */
	if (!t->MemBlock) {
		while (1)
		{
			mgaTextureObjectPtr tmp = mmesa->TexObjList[heap].prev;

			t->MemBlock = mmAllocMem( mmesa->texHeap[heap], 
						  t->totalSize,
						  6, 0 ); 
			if (t->MemBlock)
				break;

			if (mmesa->TexObjList[heap].prev->bound) {
				fprintf(stderr, 
					"Hit bound texture in upload\n"); 
				return -1;
			}

			if (mmesa->TexObjList[heap].prev == 
			    &(mmesa->TexObjList[heap])) 
			{
				fprintf(stderr, "Failed to upload texture, "
					"sz %d\n", t->totalSize);
				mmDumpMemInfo( mmesa->texHeap[heap] );
				return -1;
			}
	 
			mgaDestroyTexObj( mmesa, tmp );
		}
 
		ofs = t->MemBlock->ofs 
			+ mmesa->mgaScreen->textureOffset[heap]
			;

		t->Setup[MGA_TEXREG_ORG]  = ofs;
		t->Setup[MGA_TEXREG_ORG1] = ofs + t->offsets[1];
		t->Setup[MGA_TEXREG_ORG2] = ofs + t->offsets[2];
		t->Setup[MGA_TEXREG_ORG3] = ofs + t->offsets[3];
		t->Setup[MGA_TEXREG_ORG4] = ofs + t->offsets[4];

		mmesa->dirty |= MGA_UPLOAD_CTX;
	}

	/* Let the world know we've used this memory recently.
	 */
	mgaUpdateTexLRU( mmesa, t );

	
	if (MGA_DEBUG&DEBUG_VERBOSE_LRU)
		fprintf(stderr, "dispatch age: %d age freed memory: %d\n",
			GET_DISPATCH_AGE(mmesa), mmesa->dirtyAge);

	if (mmesa->dirtyAge >= GET_DISPATCH_AGE(mmesa)) 
		mgaWaitAgeLocked( mmesa, mmesa->dirtyAge );

	if (t->dirty_images) {
  		if (MGA_DEBUG&DEBUG_VERBOSE_LRU)
			fprintf(stderr, "*");

		for (i = 0 ; i <= t->lastLevel ; i++)
			if (t->dirty_images & (1<<i)) 
				mgaUploadTexLevel( mmesa, t, i );
	}


	t->dirty_images = 0;
	return 0;
}

/*
============================================================================

PUBLIC MGA FUNCTIONS

============================================================================
*/



static void mgaUpdateTextureEnvG200( GLcontext *ctx )
{
	struct gl_texture_object *tObj = ctx->Texture.Unit[0].Current;
	mgaTextureObjectPtr t;

	if (!tObj || !tObj->DriverData)
		return;

	t = (mgaTextureObjectPtr)tObj->DriverData;

	switch (ctx->Texture.Unit[0].EnvMode) {
	case GL_REPLACE:
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_MODULATE:
		t->Setup[MGA_TEXREG_CTL] |= TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_DECAL:
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_BLEND:
		t->ctx->Fallback |= MGA_FALLBACK_TEXTURE;
		break;
	default:
		break;
	}
}

static void mgaUpdateTextureStage( GLcontext *ctx, int unit )
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	GLuint *reg = &mmesa->Setup[MGA_CTXREG_TDUAL0 + unit];
	GLuint source = mmesa->tmu_source[unit];
	struct gl_texture_object *tObj = ctx->Texture.Unit[source].Current;

	*reg = 0;
	if (unit == 1)
		*reg = mmesa->Setup[MGA_CTXREG_TDUAL0];
	
	if ( tObj != ctx->Texture.Unit[source].CurrentD[2] ) 
		return;
	
	if ( ((ctx->Enabled>>(source*4))&TEXTURE0_ANY) != TEXTURE0_2D ) 
		return;
	
	if (!tObj || !tObj->Complete) 
		return;

	switch (ctx->Texture.Unit[source].EnvMode) {
	case GL_REPLACE:
		*reg = (TD0_color_sel_arg1 |
			TD0_alpha_sel_arg1 );
		break;

	case GL_MODULATE:
		if (unit == 0)
			*reg = ( TD0_color_arg2_diffuse |
				 TD0_color_sel_mul | 
				 TD0_alpha_arg2_diffuse |
				 TD0_alpha_sel_mul);
		else
			*reg = ( TD0_color_arg2_prevstage |
				 TD0_color_alpha_prevstage |
				 TD0_color_sel_mul | 
				 TD0_alpha_arg2_prevstage |
				 TD0_alpha_sel_mul);
		break;
	case GL_DECAL:
		*reg = (TD0_color_arg2_fcol | 
			TD0_color_alpha_currtex |
			TD0_color_alpha2inv_enable |
			TD0_color_arg2mul_alpha2 |
			TD0_color_arg1mul_alpha1 |
			TD0_color_add_add |
			TD0_color_sel_add |
			TD0_alpha_arg2_fcol |
			TD0_alpha_sel_arg2 );
		break;

	case GL_ADD:
		if (unit == 0)
			*reg = ( TD0_color_arg2_diffuse |
				 TD0_color_add_add |
				 TD0_color_sel_add | 
				 TD0_alpha_arg2_diffuse |
				 TD0_alpha_sel_add);
		else
			*reg = ( TD0_color_arg2_prevstage |
				 TD0_color_alpha_prevstage |
				 TD0_color_add_add |
				 TD0_color_sel_add | 
				 TD0_alpha_arg2_prevstage |
				 TD0_alpha_sel_add);
		break;

	case GL_BLEND:
		if (0)
			fprintf(stderr, "GL_BLEND unit %d flags %x\n", unit,
				mmesa->blend_flags);

		if (mmesa->blend_flags) 
			mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
			return;

		/* Do singletexture GL_BLEND with 'all ones' env-color
		 * by using both texture units.  Multitexture gl_blend
		 * is a fallback.  
		 */
		if (unit == 0) {
			/* Part 1: R1 = Rf ( 1 - Rt )
			 *         A1 = Af At
			 */
			*reg = ( TD0_color_arg2_diffuse |
				 TD0_color_arg1_inv_enable |
				 TD0_color_sel_mul | 
				 TD0_alpha_arg2_diffuse |
				 TD0_alpha_sel_arg1);
		} else {
			/* Part 2: R2 = R1 + Rt
			 *         A2 = A1
			 */
			*reg = ( TD0_color_arg2_prevstage |
				 TD0_color_add_add | 
				 TD0_color_sel_add | 
				 TD0_alpha_arg2_prevstage |
				 TD0_alpha_sel_arg2);		
		}	

		break;
	default:
		break;
	}
}



static void mgaUpdateTextureObject( GLcontext *ctx, int unit ) {
	mgaTextureObjectPtr t;
	struct gl_texture_object	*tObj;
	GLuint enabled;
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	GLuint source = mmesa->tmu_source[unit];

	mgaMsg(15,"mgaUpdateTextureState %d\n", unit);
	
	/* disable texturing until it is known to be good */
	mmesa->Setup[MGA_CTXREG_DWGCTL] &= DC_opcod_MASK;
	mmesa->Setup[MGA_CTXREG_DWGCTL] |= DC_opcod_trap;

	enabled = (ctx->Texture.Enabled>>(source*4))&TEXTURE0_ANY;
	if (enabled != TEXTURE0_2D) {
		if (enabled)
			mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
		return;
	}

	tObj = ctx->Texture.Unit[source].Current;

	if ( !tObj || tObj != ctx->Texture.Unit[source].CurrentD[2] ) 
		return;

/*  	fprintf(stderr, "unit %d: %d\n", unit, tObj->Name); */
	
	/* if the texture object doesn't exist at all (never used or
	   swapped out), create it now, uploading all texture images */

	if ( !tObj->DriverData ) {
		/* clear the current pointer so that texture object can be
		   swapped out if necessary to make room */
		mgaCreateTexObj( mmesa, tObj );

		if ( !tObj->DriverData ) {
			mgaMsg( 5, "mgaUpdateTextureState: create failed\n" );
			mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
			return;		/* can't create a texture object */
		}
	}

	/* we definately have a valid texture now */
	mmesa->Setup[MGA_CTXREG_DWGCTL] &= DC_opcod_MASK;
	mmesa->Setup[MGA_CTXREG_DWGCTL] |= DC_opcod_texture_trap;

	t = (mgaTextureObjectPtr)tObj->DriverData;

	if (t->dirty_images) 
		mmesa->dirty |= (MGA_UPLOAD_TEX0IMAGE << unit);

	mmesa->CurrentTexObj[unit] = t;
	t->bound = unit+1;

	if (t->MemBlock)
		mgaUpdateTexLRU( mmesa, t );


	t->Setup[MGA_TEXREG_CTL2] &= ~TMC_dualtex_enable;
	if (ctx->Texture.Enabled == (TEXTURE0_2D|TEXTURE1_2D))
		t->Setup[MGA_TEXREG_CTL2] |= TMC_dualtex_enable;

	t->Setup[MGA_TEXREG_CTL2] &= ~TMC_specen_enable;
	if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
		t->Setup[MGA_TEXREG_CTL2] |= TMC_specen_enable;
       
}






/* The G400 is now programmed quite differently wrt texture environment.
 */
void mgaUpdateTextureState( GLcontext *ctx )
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mmesa->Fallback &= ~MGA_FALLBACK_TEXTURE;

	if (mmesa->CurrentTexObj[0]) mmesa->CurrentTexObj[0]->bound = 0;
	if (mmesa->CurrentTexObj[1]) mmesa->CurrentTexObj[1]->bound = 0;
	mmesa->CurrentTexObj[0] = 0;
	mmesa->CurrentTexObj[1] = 0;   

	if (MGA_IS_G400(mmesa)) {
		mgaUpdateTextureObject( ctx, 0 );		
		mgaUpdateTextureStage( ctx, 0 );

		mmesa->Setup[MGA_CTXREG_TDUAL1] = 
			mmesa->Setup[MGA_CTXREG_TDUAL0];

		if (mmesa->multitex) {
			mgaUpdateTextureObject( ctx, 1 );	
			mgaUpdateTextureStage( ctx, 1 );
		}
		
		mmesa->dirty |= MGA_UPLOAD_TEX0 | MGA_UPLOAD_TEX1;
	} else {
		mgaUpdateTextureObject( ctx, 0 );
		mgaUpdateTextureEnvG200( ctx );		
	}

	/* schedule the register writes */
	mmesa->dirty |= MGA_UPLOAD_CTX | MGA_UPLOAD_TEX0;
}



/*
============================================================================

Driver functions called directly from mesa

============================================================================
*/

/*
 * mgaTexEnv
 */      
void mgaTexEnv( GLcontext *ctx, GLenum target, GLenum pname,
                const GLfloat *param ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT(ctx);
	mgaMsg( 10, "mgaTexEnv( %i )\n", pname );


	if (pname == GL_TEXTURE_ENV_MODE) {
		/* force the texture state to be updated */
		FLUSH_BATCH( MGA_CONTEXT(ctx) );
		MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
	}
	else if (pname == GL_TEXTURE_ENV_COLOR)
	{
		struct gl_texture_unit *texUnit = 
			&ctx->Texture.Unit[ctx->Texture.CurrentUnit];
		GLfloat *fc = texUnit->EnvColor;
		GLubyte c[4];
		GLuint col;

		
		c[0] = fc[0];
		c[1] = fc[1];
		c[2] = fc[2];
		c[3] = fc[3];

		/* No alpha at 16bpp?
		 */
		col = mgaPackColor( mmesa->mgaScreen->Attrib,
				    c[0], c[1], c[2], c[3] );

  		mmesa->envcolor = (c[3]<<24) | (c[0]<<16) | (c[1]<<8) | (c[2]); 
    
		if (mmesa->Setup[MGA_CTXREG_FCOL] != col) {
			FLUSH_BATCH(mmesa);	
			mmesa->Setup[MGA_CTXREG_FCOL] = col;      
			mmesa->dirty |= MGA_UPLOAD_CTX;

			mmesa->blend_flags &= ~MGA_BLEND_ENV_COLOR;

			/* Actually just require all four components to be
			 * equal.  This permits a single-pass GL_BLEND.
			 * 
			 * More complex multitexture/multipass fallbacks
			 * for blend can be done later.
			 */
			if (mmesa->envcolor != 0x0 &&
			    mmesa->envcolor != 0xffffffff)
				mmesa->blend_flags |= MGA_BLEND_ENV_COLOR;
		}
	}

}

/*
 * mgaTexImage
 */ 
void mgaTexImage( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj, GLint level,
		  GLint internalFormat,
		  const struct gl_texture_image *image ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );	
	mgaTextureObjectPtr t;

	mgaMsg( 10,"mgaTexImage( %p, level %i )\n", tObj, level );
  
 	/* just free the mga texture if it exists, it will be recreated at
	mgaUpdateTextureState time. */  
	t = (mgaTextureObjectPtr) tObj->DriverData;
	if ( t ) {
		if (t->bound) FLUSH_BATCH(mmesa);
		/* if this is the current object, it will force an update */
 	 	mgaDestroyTexObj( mmesa, t );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}
}

/*
 * mgaTexSubImage
 */      
void mgaTexSubImage( GLcontext *ctx, GLenum target,
		     struct gl_texture_object *tObj, GLint level,
		     GLint xoffset, GLint yoffset,
		     GLsizei width, GLsizei height,
		     GLint internalFormat,
		     const struct gl_texture_image *image ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mgaTextureObjectPtr t;

	mgaMsg(10,"mgaTexSubImage() Size: %d,%d of %d,%d; Level %d\n",
		width, height, image->Width,image->Height, level);

	t = (mgaTextureObjectPtr) tObj->DriverData;


 	/* just free the mga texture if it exists, it will be recreated at
	mgaUpdateTextureState time. */  
	t = (mgaTextureObjectPtr) tObj->DriverData;
	if ( t ) {
		if (t->bound) FLUSH_BATCH(mmesa);
		/* if this is the current object, it will force an update */
 	 	mgaDestroyTexObj( mmesa, t );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}



#if 0
	/* the texture currently exists, so directly update it */
	mgaUploadSubImage( t, level, xoffset, yoffset, width, height );
#endif
}

/*
 * mgaTexParameter
 * This just changes variables and flags for a state update, which
 * will happen at the next mgaUpdateTextureState
 */
void mgaTexParameter( GLcontext *ctx, GLenum target,
		      struct gl_texture_object *tObj,
		      GLenum pname, const GLfloat *params )
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mgaTextureObjectPtr t;

	mgaMsg( 10, "mgaTexParameter( %p, %i )\n", tObj, pname );

	t = (mgaTextureObjectPtr) tObj->DriverData;

	/* if we don't have a hardware texture, it will be automatically
	created with current state before it is used, so we don't have
	to do anything now */
	if ( !t || target != GL_TEXTURE_2D ) {
		return;
	}

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
		if (t->bound) FLUSH_BATCH(mmesa);
		mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
		if (t->bound) FLUSH_BATCH(mmesa);
		mgaSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
		break;
  
	case GL_TEXTURE_BORDER_COLOR:
		if (t->bound) FLUSH_BATCH(mmesa);
		mgaSetTexBorderColor(t,tObj->BorderColor);
		break;

	default:
		return;
	}

	mmesa->new_state |= MGA_NEW_TEXTURE;
}

/*
 * mgaBindTexture
 */      
void mgaBindTexture( GLcontext *ctx, GLenum target,
		     struct gl_texture_object *tObj ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );

	mgaMsg( 10, "mgaBindTexture( %p )\n", tObj );

	FLUSH_BATCH(mmesa);

	if (mmesa->CurrentTexObj[ctx->Texture.CurrentUnit]) {
		mmesa->CurrentTexObj[ctx->Texture.CurrentUnit]->bound = 0;
		mmesa->CurrentTexObj[ctx->Texture.CurrentUnit] = 0;  
	}

	/* force the texture state to be updated 
	 */
	MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
}

/*
 * mgaDeleteTexture
 */      
void mgaDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mgaTextureObjectPtr t = (mgaTextureObjectPtr)tObj->DriverData;

	mgaMsg( 10, "mgaDeleteTexture( %p )\n", tObj );
	
	if ( t ) {
		if (t->bound) {
			FLUSH_BATCH(mmesa);
			mmesa->CurrentTexObj[t->bound-1] = 0;
			mmesa->new_state |= MGA_NEW_TEXTURE;
		}

		mgaDestroyTexObj( mmesa, t );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}
}


/* Have to grab the lock to find out if anyone has kicked out our
 * textures.
 */
GLboolean mgaIsTextureResident( GLcontext *ctx, struct gl_texture_object *t ) 
{
	mgaTextureObjectPtr mt;

/*  	LOCK_HARDWARE; */
	mt = (mgaTextureObjectPtr)t->DriverData;
/*  	UNLOCK_HARDWARE; */

	return mt && mt->MemBlock;
}

void mgaDDInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.TexEnv = mgaTexEnv;
   ctx->Driver.TexImage = mgaTexImage;
   ctx->Driver.TexSubImage = mgaTexSubImage;
   ctx->Driver.BindTexture = mgaBindTexture;
   ctx->Driver.DeleteTexture = mgaDeleteTexture;
   ctx->Driver.TexParameter = mgaTexParameter;
   ctx->Driver.UpdateTexturePalette = 0;
   ctx->Driver.IsTextureResident = mgaIsTextureResident;
}
