#ifndef _XF86XVMC_H
#define _XF86XVMC_H

#include "xvmc.h"
#include "xf86xv.h"

typedef struct {
  int num_xvimages;
  int *xvimage_ids;
} XF86MCImageIDList; 

typedef struct {
  int surface_type_id;
  int chroma_format;
  int color_description;
  unsigned short max_width;       
  unsigned short max_height;   
  unsigned short subpicture_max_width;
  unsigned short subpicture_max_height;
  int mc_type;         
  int flags;
  XF86MCImageIDList *compatible_subpictures; 
} XF86MCSurfaceInfoRec, *XF86MCSurfaceInfoPtr;

typedef int (*xf86XvMCCreateContextProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCContextPtr context,
  int *num_priv,
  CARD32 **priv 
);

typedef void (*xf86XvMCDestroyContextProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCContextPtr context
);

typedef int (*xf86XvMCCreateSurfaceProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCSurfacePtr surface,
  int *num_priv,
  CARD32 **priv
);

typedef void (*xf86XvMCDestroySurfaceProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCSurfacePtr surface
);

typedef int (*xf86XvMCCreateSubpictureProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCSubpicturePtr subpicture,
  int *num_priv,
  CARD32 **priv
);

typedef void (*xf86XvMCDestroySubpictureProcPtr) (
  ScrnInfoPtr pScrn,
  XvMCSubpicturePtr subpicture
);


typedef struct {
  char *name;
  int num_surfaces;
  XF86MCSurfaceInfoPtr *surfaces;
  int num_subpictures;
  XF86ImagePtr *subpictures;
  xf86XvMCCreateContextProcPtr 		CreateContext; 
  xf86XvMCDestroyContextProcPtr		DestroyContext; 
  xf86XvMCCreateSurfaceProcPtr 		CreateSurface; 
  xf86XvMCDestroySurfaceProcPtr		DestroySurface; 
  xf86XvMCCreateSubpictureProcPtr	CreateSubpicture; 
  xf86XvMCDestroySubpictureProcPtr	DestroySubpicture; 
} XF86MCAdaptorRec, *XF86MCAdaptorPtr;

Bool xf86XvMCScreenInit(
  ScreenPtr pScreen, 
  int num_adaptors,
  XF86MCAdaptorPtr *adaptors
);

#endif /* _XF86XVMC_H */
