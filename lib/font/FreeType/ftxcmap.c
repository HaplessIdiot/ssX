/* $XFree86: $ */

#include "tttypes.h"
#include "ttobjs.h"
#include "tttables.h"
#include "ftxcmap.h"

static int charmap_first4(PCMap4, int*);
static int charmap_next4(PCMap4, UShort, int*);
static UShort charmap_find_id4(PCMap4, UShort, TCMap4Segment*, int);

int 
TT_CharMap_First(TT_CharMap charMap, int *id)
{
  PCMapTable cmap;
  int i,c;
  
  if(!(cmap=HANDLE_CharMap(charMap)))
     return -1;

  switch(cmap->format) {
  case 0:
    if(id)
      *id=cmap->c.cmap0.glyphIdArray[0];
    return 0;
  case 4: return charmap_first4(&cmap->c.cmap4, id);
  case 6:
    if(cmap->c.cmap6.entryCount<1)
      return -1;
    if(id)
      *id=cmap->c.cmap6.glyphIdArray[0];
    return cmap->c.cmap6.firstCode;
  default:
    for(i=0; i<=0xFFFF; i++)
      if((c=TT_Char_Index(charMap, i))>0) {
        if(id)
          *id=c;
        return i;
      }
    return -1;
  }
}

static int
charmap_first4(PCMap4 cmap4, int* id)
{
  UShort firstCode;

  if(cmap4->segCountX2/2<1)
    return -1;

  firstCode=cmap4->segments[0].startCount;

  if(id)
    *id=charmap_find_id4(cmap4, firstCode, &(cmap4->segments[0]), 0);
  return firstCode;
}

int 
TT_CharMap_Next(TT_CharMap charMap, int index, int *id)
{
  PCMapTable cmap;
  int i,c;
  
  if(!(cmap=HANDLE_CharMap(charMap)))
     return -1;

  switch(cmap->format) {
  case 0: 
    if(index<255) {
      if(id)
        *id=cmap->c.cmap0.glyphIdArray[index+1];
      return index+1;
    } else
      return -1;
  case 4: return charmap_next4(&cmap->c.cmap4, index, id);
  case 6: 
    { 
      int firstCode=cmap->c.cmap6.firstCode;
      if(index+1 < firstCode + cmap->c.cmap6.entryCount) {
        if(id)
          *id=cmap->c.cmap6.glyphIdArray[index+1-firstCode];
        return index+1;
      } else
        return -1;
    }
  default:
    for(i=index; i<=0xFFFF; i++)
      if((c=TT_Char_Index(charMap, i))>0) {
        if(id)
          *id=c;
        return i;
      }
    return -1;
  }
}

static int charmap_next4(PCMap4 cmap4, UShort charCode, int *id)
{
  UShort         segCount, nextCode;
  int            i;
  TCMap4Segment  seg4;

  if(charCode==0xFFFF)
    return -1;                /* get it out of the way now */

  segCount = cmap4->segCountX2 / 2;

  for(i=0; i<segCount; i++)
    if(charCode < cmap4->segments[i].endCount)
      break;

  /* Safety check - even though the last endCount should be 0xFFFF */
  if(i>=segCount)
    return -1;

  seg4 = cmap4->segments[i];

  if(charCode<seg4.startCount)
    nextCode=seg4.startCount;
  else
    nextCode=charCode+1;
    
  if(id)
    *id=charmap_find_id4(cmap4, nextCode, &seg4, i);

  return nextCode;
}

static UShort
charmap_find_id4(PCMap4 cmap4, UShort charCode, TCMap4Segment *seg4, int i)
{
  UShort index1;

  if (seg4->idRangeOffset==0)
    return (charCode+seg4->idDelta) & 0xFFFF;
  else {
    index1 = 
      seg4->idRangeOffset/2+(charCode-seg4->startCount)-
      (cmap4->segCountX2/2-i);
    if(cmap4->glyphIdArray[index1]==0)
      return 0;
    else
      return (cmap4->glyphIdArray[index1]+seg4->idDelta) & 0xFFFF;
  }
}
