/* $XFree86$ */

#ifndef _UTILS_H_
#define _UTILS_H_

/* from utils.c */
extern Bool lbxZeroPad;
extern Bool lbxWinAttr;
extern Bool lbxDoCmapGrabbing;

extern int lbxMaxMotionEvents;
extern int zlevel;

extern char *atomsFile;

extern char *rgbPath;

/* from wire.c */
extern Bool reconnectAfterCloseServer;

/* from tags.c */
extern int lbxTagCacheSize;

/* from lbxutil.c */
extern Bool compStats;

/* from os/conncetion.c */
extern Bool PartialNetwork;

#endif /* _UTILS_H_ */
