/*************************************************************************
 *
 *   HWDiff.c 
 *
 *   Implement all Video Function for the Driver 
 *
 *   DATE     04/07/2003
 *
 *************************************************************************/

#include "via_driver.h" 
#include "HWDiff.h"
/*#include "debug.h"*/


/**************************************************************************
//Global Variable for Video function
**************************************************************************/
VIDHWDIFFERENCE VideoHWDifference;

/**************************************************************************
//Extern Variable for Video function
**************************************************************************/


/**************************************************************************
//Extern function for Video function
**************************************************************************/


/**************************************************************************
//Defination for Video function
************************************************************************/


/**************************************************************************
//Structure defination for Video function
**************************************************************************/


/**************************************************************************
//function defination for Video Driver
**************************************************************************/


/**************************************************************************
//function implement for Video Driver
**************************************************************************/
void VIAvfInitHWDiff( VIAPtr  pVia )
{
    LPVIDHWDIFFERENCE lpVideoHWDifference = &VideoHWDifference;
    /*LPWaitHWINFO lpWaitHWInfo;
    
    //lpVideoHWDifference = (LPVIDHWDIFFERENCE) (ppdev->lpVideoHWDifference);
    //lpWaitHWInfo = (LPWaitHWINFO) ppdev->lpWaitHWInfo;*/
    
    /*switch(ppdev->dwDeviceID)
    {
        case DEVICE_VT3204:
             //HW Difference Flag
             lpVideoHWDifference->dwThreeHQVBuffer = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3SrcHeightSetting = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwSupportExtendFIFO = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwHQVFetchByteUnit = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwHQVInitPatch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwSupportV3Gamma=VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwUpdFlip = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwHQVDisablePatch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwSUBFlip = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwNeedV3Prefetch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwNeedV4Prefetch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwUseSystemMemory = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwExpandVerPatch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwExpandVerHorPatch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwV3ExpireNumTune = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3FIFOThresholdTune = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwCheckHQVFIFOEmpty = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwUseMPEGAGP = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3FIFOPatch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwSupportTwoColorKey = VID_HWDIFF_FALSE; 
             break;
        case DEVICE_VT3205:
             //HW Difference Flag
             lpVideoHWDifference->dwThreeHQVBuffer = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3SrcHeightSetting = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwSupportExtendFIFO = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwHQVFetchByteUnit = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwHQVInitPatch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwSupportV3Gamma=VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwUpdFlip = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwHQVDisablePatch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwSUBFlip = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwNeedV3Prefetch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwNeedV4Prefetch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwUseSystemMemory = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwExpandVerPatch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwExpandVerHorPatch = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3ExpireNumTune = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3FIFOThresholdTune = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwCheckHQVFIFOEmpty = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwUseMPEGAGP = VID_HWDIFF_TRUE;
             lpVideoHWDifference->dwV3FIFOPatch = VID_HWDIFF_FALSE;
             lpVideoHWDifference->dwSupportTwoColorKey = VID_HWDIFF_FALSE;
             break;
        case VIA_DEVICE_CLE1:
        case VIA_DEVICE_CLE2:*/
             switch (pVia->ChipRev)
             {
                case VIA_REVISION_CLEC0:
                case VIA_REVISION_CLEC1:
                     /*HW Difference Flag*/
                     lpVideoHWDifference->dwThreeHQVBuffer = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwV3SrcHeightSetting = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwSupportExtendFIFO = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwHQVFetchByteUnit = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwHQVInitPatch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwSupportV3Gamma=VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwUpdFlip = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwHQVDisablePatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwSUBFlip = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwNeedV3Prefetch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwNeedV4Prefetch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwUseSystemMemory = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwExpandVerPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwExpandVerHorPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwV3ExpireNumTune = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwV3FIFOThresholdTune = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwCheckHQVFIFOEmpty = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwUseMPEGAGP = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwV3FIFOPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwSupportTwoColorKey = VID_HWDIFF_TRUE;
                     /*lpVideoHWDifference->dwCxColorSpace = VID_HWDIFF_TRUE;*/
                     break;
                default:
                     /*HW Difference Flag*/
                     lpVideoHWDifference->dwThreeHQVBuffer = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwV3SrcHeightSetting = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwSupportExtendFIFO = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwHQVFetchByteUnit = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwHQVInitPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwSupportV3Gamma=VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwUpdFlip = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwHQVDisablePatch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwSUBFlip = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwNeedV3Prefetch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwNeedV4Prefetch = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwUseSystemMemory = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwExpandVerPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwExpandVerHorPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwV3ExpireNumTune = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwV3FIFOThresholdTune = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwCheckHQVFIFOEmpty = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwUseMPEGAGP = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwV3FIFOPatch = VID_HWDIFF_TRUE;
                     lpVideoHWDifference->dwSupportTwoColorKey = VID_HWDIFF_FALSE;
                     lpVideoHWDifference->dwCxColorSpace = VID_HWDIFF_FALSE;
                     break;
             }/*CLEC0 Switch*/
/*             break;             
    }    */
}     
