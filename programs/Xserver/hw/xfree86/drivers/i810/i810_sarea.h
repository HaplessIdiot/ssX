#ifndef I810_SAPRIV_H
#define I810_SAPRIV_H

/* Each region is a minimum of 32k, and there are at most 128 of them.
 */
#define I810_NR_TEX_REGIONS 128

 

typedef struct {
   unsigned char next, prev;	/* array indices to form a circular LRU list */
   unsigned char in_use;	/* owned by a client, or free? */
   int age;			/* tracked by clients to update local LRU's */
} i810TexRegion;

typedef struct {
   int ringOwner;		             /* not used */
   int ctxOwner;		             /* last context to upload state */

   /* Maintain an LRU of contiguous regions of texture space.  If
    * you think you own a region of texture memory, and it has an age
    * different to the one you set, then you are mistaken and it has
    * been stolen by another client.  If texAge hasn't changed, there 
    * is no need to walk the list.
    *
    * These regions can be used as a proxy for the fine-grained texture
    * information of other clients - by maintaining them in the same
    * lru which is used to age their own textures, clients have an
    * approximate lru for the whole of global texture space, and can
    * make informed decisions as to which areas to kick out.  There is
    * no need to choose whether to kick out your own texture or someone
    * else's - simply eject them all in LRU order.
    */
   i810TexRegion texList[I810_NR_TEX_REGIONS+1]; /* Last element is sentinal */
   int texAge;	                                 /* Current age counter */


   int lastWrap;
   int lastSync;
   int ringAge;

} I810SAREAPriv;

#endif
