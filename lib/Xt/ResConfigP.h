/* $TOG: ResConfigP.h /main/1 1997/10/13 12:25:58 barstow $ */
/*
 * 1.2 src/gos/2d/XTOP/lib/Xt/custom_proto.h, xtoolkit, gos42G 8/12/93 10:35:50 
 *
 * COMPONENT_NAME:  XTOOLKIT
 *
 * FUNCTIONS:
 *
 * ORIGINS:  27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1992 
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#ifndef _RESCONFIGP_H
#define _RESCONFIGP_H

/*
 * Atom names for resource configuration management customization tool.
 */
#define RCM_DATA "Custom Data"
#define RCM_INIT "Custom Init"

extern void _XtResourceConfigurationEH(
#if NeedFunctionPrototypes
	Widget 		/* w */, 
	XtPointer 	/* client_data */, 
	XEvent * 	/* event */
#endif
);

#endif
