/*
 *  video4linux Xv Driver 
 *  based on Michael Schimek's permedia 2 driver.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/v4l/v4l.c,v 1.3 1999/04/04 08:46:20 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86fbman.h"
#include "xf86xv.h"
#include "Xv.h"
#include "miscstruct.h"
#include "dgaproc.h"
#include "xf86str.h"


#include <asm/ioctl.h>		/* _IORW(xxx) #defines are here */
#if 1
typedef unsigned long ulong;
#endif
#include "videodev.h"


#define DEBUG(x) (x)

static void     V4LIdentify(int flags);
static Bool     V4LProbe(DriverPtr drv, int flags);
                  
DriverRec V4L = {
        40000,
        "Xv driver for video4linux",
        V4LIdentify, /* Identify*/
        V4LProbe, /* Probe */
        NULL,
        0
};    


#ifdef XFree86LOADER

static MODULESETUPPROTO(v4lSetup);

static XF86ModuleVersionInfo v4lVersRec =
{
        "v4l",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0, 0, 1,
        ABI_CLASS_VIDEODRV,
        ABI_VIDEODRV_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}
};

XF86ModuleData v4lModuleData = { &v4lVersRec, v4lSetup, NULL };

static pointer
v4lSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
        const char *osname;
	static Bool setupDone = FALSE;

	if (setupDone) {
	    if (errmaj)
		*errmaj = LDR_ONCEONLY;
	    return NULL;             
	}
	
	setupDone = TRUE;

        /* Check that we're being loaded on a Linux system */
        LoaderGetOS(&osname, NULL, NULL, NULL);
        if (!osname || strcmp(osname, "linux") != 0) {
                if (errmaj)
                        *errmaj = LDR_BADOS;
                if (errmin)
                        *errmin = 0;
                return NULL;
        } else {
                /* OK */

	        xf86AddDriver (&V4L, module, 0);   
	  
                return (pointer)1;
        }
}

#else

#include <fcntl.h>
#include <sys/ioctl.h>

#endif

typedef struct _PortPrivRec {
    ScrnInfoPtr                 pScrn;
    FBAreaPtr			pFBArea[2];
    int				VideoOn;	/* No, Once, Yes */
    Bool			StreamOn;

    /* file handle */
    int 			fd;
    char                        devname[16];
    int                         useCount;

    /* overlay */
    struct video_buffer		ov_fbuf;
    struct video_window		ov_win;

    /* attributes */
    struct video_picture	pict;

    XF86VideoEncodingPtr        enc;
} PortPrivRec, *PortPrivPtr;

#define XV_ENCODING	"XV_ENCODING"
#define XV_BRIGHTNESS  	"XV_BRIGHTNESS"
#define XV_CONTRAST 	"XV_CONTRAST"
#define XV_SATURATION  	"XV_SATURATION"
#define XV_HUE		"XV_HUE"

#define XV_FREQ		"XV_FREQ"

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvEncoding, xvBrightness, xvContrast, xvSaturation, xvHue, xvFreq;

static XF86VideoFormatRec
InputVideoFormats[] = {
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

static int V4lOpenDevice(PortPrivPtr pPPriv, ScrnInfoPtr pScrn)
{
    pPPriv->useCount++;

    if (pPPriv->fd == -1) {
	pPPriv->fd = open(pPPriv->devname, O_RDWR, 0);

	pPPriv->ov_fbuf.width        = pScrn->virtualX;
	pPPriv->ov_fbuf.height       = pScrn->virtualY;
	pPPriv->ov_fbuf.depth        = pScrn->bitsPerPixel;
	pPPriv->ov_fbuf.bytesperline = pScrn->displayWidth * ((pScrn->bitsPerPixel + 7)/8);
	pPPriv->ov_fbuf.base         = (pointer)(pScrn->memPhysBase + pScrn->fbOffset);
	
	if (-1 == ioctl(pPPriv->fd,VIDIOCSFBUF,&(pPPriv->ov_fbuf)))
	    perror("ioctl VIDIOCSFBUF");
    }

    if (pPPriv->fd == -1)
	return errno;
   
    return 0;
}

static void V4lCloseDevice(PortPrivPtr pPPriv)
{
    pPPriv->useCount--;
    
    if(pPPriv->useCount == 0 && pPPriv->fd != -1) {
	close(pPPriv->fd);
	pPPriv->fd = -1;
    }
}


static int
V4lPutVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    struct video_clip *clip;
    BoxPtr pBox;
    int i;
    int one=1;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/PV\n"));

    /* ignore vid-* for now */

    /* window */
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "  win: %dx%d+%d+%d\n",
		drw_w,drw_h,drw_x,drw_y));
    pPPriv->ov_win.x      = drw_x;
    pPPriv->ov_win.y      = drw_y;
    pPPriv->ov_win.width  = drw_w;
    pPPriv->ov_win.height = drw_h;
    pPPriv->ov_win.flags  = 0;
 
    /* clipping */
    if (pPPriv->ov_win.clips) {
	xfree(pPPriv->ov_win.clips);
	pPPriv->ov_win.clips = NULL;
    }
    pPPriv->ov_win.clipcount = REGION_NUM_RECTS(clipBoxes);
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,"  clip: have #%d\n",
		pPPriv->ov_win.clipcount));
    if (0 != pPPriv->ov_win.clipcount) {
	pPPriv->ov_win.clips = xalloc(pPPriv->ov_win.clipcount*sizeof(struct video_clip));
	memset(pPPriv->ov_win.clips,0,pPPriv->ov_win.clipcount*sizeof(struct video_clip));
	pBox = REGION_RECTS(clipBoxes);
	clip = pPPriv->ov_win.clips;
	for (i = 0; i < REGION_NUM_RECTS(clipBoxes); i++, pBox++, clip++) {
	    clip->x	 = pBox->x1 - drw_x;
	    clip->y      = pBox->y1 - drw_y;
	    clip->width  = pBox->x2 - pBox->x1;
	    clip->height = pBox->y2 - pBox->y1;
	}
    }

    /* Open a file handle to the device */
    
    V4lOpenDevice(pPPriv, pScrn);

    /* start */

    if (-1 == ioctl(pPPriv->fd,VIDIOCSWIN,&(pPPriv->ov_win)))
	perror("ioctl VIDIOCSWIN");
    if (-1 == ioctl(pPPriv->fd, VIDIOCCAPTURE, &one))
	perror("ioctl VIDIOCCAPTURE(1)");

    pPPriv->VideoOn = 1;
    return Success;
}

static int
V4lPutStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/PS\n"));

    /* FIXME */
    return Success;
}

static void
V4lStopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    int zero=0;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/StopVideo\n"));

    if (-1 == ioctl(pPPriv->fd, VIDIOCCAPTURE, &zero))
	perror("ioctl VIDIOCCAPTURE(0)");

    V4lCloseDevice(pPPriv);
    pPPriv->VideoOn = 0;
}

/* v4l uses range 0 - 65535; Xv uses -1000 - 1000 */
static int
v4l_to_xv(int val) {
    val = val * 2000 / 65536 - 1000;
    if (val < -1000) val = -1000;
    if (val >  1000) val =  1000;
    return val;
}
static int
xv_to_v4l(int val) {
    val = val * 65536 / 2000 + 32768;
    if (val <    -0) val =     0;
    if (val > 65535) val = 65535;
    return val;
}

static int
V4lSetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data; 
    struct video_channel chan;
    int ret = Success;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/SPA %d, %d\n",
	attribute, value));

    V4lOpenDevice(pPPriv, pScrn);

    if (-1 == pPPriv->fd) {
	ret = Success /* FIXME: EBUSY/ENODEV ?? */;
    } else if (attribute == xvEncoding) {
	chan.channel = value/3;
	chan.norm    = value%3;
	if (-1 == ioctl(pPPriv->fd,VIDIOCSCHAN,&chan))
	    perror("ioctl VIDIOCSCHAN");
    } else if (attribute == xvBrightness ||
               attribute == xvContrast   ||
               attribute == xvSaturation ||
               attribute == xvHue) {
	ioctl(pPPriv->fd,VIDIOCGPICT,&pPPriv->pict);
	if (attribute == xvBrightness) pPPriv->pict.brightness = xv_to_v4l(value);
	if (attribute == xvContrast)   pPPriv->pict.contrast   = xv_to_v4l(value);
	if (attribute == xvSaturation) pPPriv->pict.colour     = xv_to_v4l(value);
	if (attribute == xvHue)        pPPriv->pict.hue        = xv_to_v4l(value);
	ioctl(pPPriv->fd,VIDIOCSPICT,&pPPriv->pict);
    } else if (attribute == xvFreq) {
	/* ErrorF("setfreq=%d\n",value); */
	if (-1 == ioctl(pPPriv->fd,VIDIOCSFREQ,&value))
	    perror("ioctl");
    } else {
	ret = BadValue;
    }

    V4lCloseDevice(pPPriv);
    return ret;
}

static int
V4lGetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    int ret = Success;

    V4lOpenDevice(pPPriv, pScrn);

    if (-1 == pPPriv->fd) {
	ret = Success /* FIXME: EBUSY/ENODEV ?? */;
    } else if (attribute == xvEncoding) {
	/* TODO */
    } else if (attribute == xvBrightness ||
               attribute == xvContrast   ||
               attribute == xvSaturation ||
               attribute == xvHue) {
	ioctl(pPPriv->fd,VIDIOCGPICT,&pPPriv->pict);
	if (attribute == xvBrightness) *value = v4l_to_xv(pPPriv->pict.brightness);
	if (attribute == xvContrast)   *value = v4l_to_xv(pPPriv->pict.contrast);
	if (attribute == xvSaturation) *value = v4l_to_xv(pPPriv->pict.colour);
	if (attribute == xvHue)        *value = v4l_to_xv(pPPriv->pict.hue);
    } else if (attribute == xvFreq) {
	ioctl(pPPriv->fd,VIDIOCGFREQ,value);
    } else {
	ret = BadValue;
    }

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/GPA %d, %d\n",
	attribute, *value));

    V4lCloseDevice(pPPriv);
    return ret;
}

static void
V4lQueryBestSize(ScrnInfoPtr pScrn, Bool motion,
    short vid_w, short vid_h, short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h, pointer data)
{
    /* FIXME */
    *p_w = drw_w;
    *p_h = drw_h;
}


static void
V4LIdentify(int flags)
{
    xf86Msg(X_INFO, "v4l driver for Video4Linux\n");
}        

static char*
fixname(char *str)
{
	int s,d;
	for (s=0, d=0;; s++) {
		if (str[s] == '-')
			continue;
		str[d++] = tolower(str[s]);
		if (0 == str[s])
			break;
	}
	return str;
}

static XF86VideoEncodingPtr
V4LBuildEncodings(int fd, int *count)
{
    static struct video_capability  cap;
    static struct video_channel     channel;
    XF86VideoEncodingPtr            enc;
    int i;

    if (-1 == ioctl(fd,VIDIOCGCAP,&cap))
	return NULL;

    enc = xalloc(sizeof(XF86VideoEncodingRec)*3*cap.channels);
    memset(enc,0,sizeof(XF86VideoEncodingRec)*3*cap.channels);

    for (i = 0; i < 3*cap.channels; ) {
	channel.channel = i/3;
	if (-1 == ioctl(fd,VIDIOCGCHAN,&channel)) {
	    perror("ioctl VIDIOCGCHAN");
	    return NULL;
	}

	/* one for PAL ... */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 768;
	enc[i].height = 576;
	enc[i].rate.numerator   =  1;
	enc[i].rate.denominator = 50;
	sprintf(enc[i].name,"pal-%s",fixname(channel.name));
	i++;

	/* NTSC */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 640;
	enc[i].height = 480;
	enc[i].rate.numerator   =  1001;
	enc[i].rate.denominator = 60000;
	sprintf(enc[i].name,"ntsc-%s",fixname(channel.name));
	i++;

	/* SECAM */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 768;
	enc[i].height = 576;
	enc[i].rate.numerator   =  1;
	enc[i].rate.denominator = 50;
	sprintf(enc[i].name,"secam-%s",fixname(channel.name));
	i++;
    }
    *count = i;
    return enc;
}


static Bool
V4LProbe(DriverPtr drv, int flags)
{
    PortPrivPtr pPPriv;
    DevUnion *Private;
    XF86VideoAdaptorPtr  VAR[4];
    XF86VideoEncodingPtr enc;
    char dev[16];
    int  fd,i,nenc;

    DEBUG(xf86Msg(X_INFO, "v4l: init start\n"));

    for (i = 0; i < 4; i++) {
	sprintf(dev,"/dev/video%d",i);
	fd = open(dev, O_RDWR, 0);
	if (fd == -1)
	    break;
	if (NULL == (enc = V4LBuildEncodings(fd,&nenc)))
	    break;
	
	DEBUG(xf86Msg(X_INFO,  "v4l: %s ok\n",dev));

	/* our private data */
	pPPriv = xalloc(sizeof(PortPrivRec));
	memset(pPPriv,0,sizeof(PortPrivRec));
	pPPriv->fd    = -1;
	strncpy(pPPriv->devname, dev, 16);
	pPPriv->useCount=0;

	/* alloc VideoAdaptorRec */
	VAR[i] = xalloc(sizeof(XF86VideoAdaptorRec));
	memset(VAR[i],0,sizeof(XF86VideoAdaptorRec));

	/* hook in private data */
	Private = xalloc(sizeof(DevUnion));
	memset(Private,0,sizeof(DevUnion));
	Private->ptr = (pointer)pPPriv;
	VAR[i]->pPortPrivates = Private;
	VAR[i]->nPorts = 1;

	/* init VideoAdaptorRec */
	VAR[i]->type  = XvInputMask;
	VAR[i]->name  = "video4linux";
	VAR[i]->flags = VIDEO_INVERT_CLIPLIST;

	VAR[i]->PutVideo = V4lPutVideo;
	VAR[i]->PutStill = V4lPutStill;
	VAR[i]->StopVideo = V4lStopVideo;
	VAR[i]->SetPortAttribute = V4lSetPortAttribute;
	VAR[i]->GetPortAttribute = V4lGetPortAttribute;
	VAR[i]->QueryBestSize = V4lQueryBestSize;

	VAR[i]->nEncodings = nenc;
	VAR[i]->pEncodings = enc;
	VAR[i]->nFormats =
		sizeof(InputVideoFormats) / sizeof(InputVideoFormats[0]);
	VAR[i]->pFormats = InputVideoFormats;
	if (fd != -1)
	    close(fd);
    }

    xvEncoding   = MAKE_ATOM(XV_ENCODING);
    xvHue        = MAKE_ATOM(XV_HUE);
    xvSaturation = MAKE_ATOM(XV_SATURATION);
    xvBrightness = MAKE_ATOM(XV_BRIGHTNESS);
    xvContrast   = MAKE_ATOM(XV_CONTRAST);

    xvFreq       = MAKE_ATOM(XV_FREQ);

    DEBUG(xf86Msg(X_INFO, "v4l: init done, %d found\n",i));
    if (i) {
	xf86XVRegisterGenericAdaptor(VAR, i);
	drv->refCount++;
    }
    return (VAR != NULL);
}
