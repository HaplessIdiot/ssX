/* $XConsortium: xkbcomp.c /main/8 1996/02/02 14:17:40 kaleb $ */
/* $XFree86: xc/programs/xkbcomp/xkbcomp.c,v 3.1 1996/01/16 15:09:04 dawes Exp $ */
/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be 
 used in advertising or publicity pertaining to distribution 
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability 
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.
 
 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include <stdio.h>
#include <ctype.h>
#include <X11/keysym.h>

#if defined(sgi)
#include <malloc.h>
#endif

#define	DEBUG_VAR_NOT_LOCAL
#define	DEBUG_VAR debugFlags
#include "xkbcomp.h"
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include "xkbpath.h"
#include "parseutils.h"
#include "misc.h"
#include "tokens.h"
#include <X11/extensions/XKBgeom.h>

#define	lowbit(x)	((x) & (-(x)))

/***====================================================================***/

#define	WANT_DEFAULT	0
#define	WANT_XKM_FILE	1
#define	WANT_C_HDR	2
#define	WANT_XKB_FILE	3
#define	WANT_X_SERVER	4
#define	WANT_LISTING	5

#define	INPUT_UNKNOWN	0
#define	INPUT_XKB	1
#define	INPUT_XKM	2

static char *fileTypeExt[] = {
	"XXX",
	"xkm",
	"h",
	"xkb",
	"dir"
};

static	unsigned	inputFormat,outputFormat;
static	char *		rootDir;
static	char *		inputFile;
static	char *		inputMap;
static	char *		outputFile;
static	char *		inDpyName;
static	char *		outDpyName;
static	Display *	inDpy;
static	Display *	outDpy;
static	Bool		showImplicit= False;
static	Bool		synch= False;
static	Bool		merge= False;
static	Bool		computeDflts= False;
static	Bool		xkblist= False;
	unsigned	warningLevel= 5;
	unsigned	verboseLevel= 0;
	unsigned	optionalParts= 0;
static	char *		preErrorMsg= NULL;
static	char *		postErrorMsg= NULL;
static	char *		errorPrefix= NULL;

#define	WantLongListing	(1<<0)
#define	WantPartialMaps	(1<<1)
#define	WantHiddenMaps	(1<<2)
#define	WantFullNames	(1<<3)

/***====================================================================***/

static	int		szListing= 0;
static	int		nListed= 0;
static	int		nFilesListed= 0;
static	char **		filesToList= NULL;
static	char **		mapsToList= NULL;

static Bool
#if NeedFunctionPrototypes
AddListing(char *file,char *map)
#else
AddListing(file,map)
    char *file;
    char *map;
#endif
{
    if (map==NULL) {
	char *tmp;
	tmp= strchr(file,'(');
	if (tmp!=NULL) {
	    map= &tmp[1];
	    tmp= strchr(file,')');
	    if ((tmp==NULL)||(tmp[1]!='\0')) {
		ERROR("Files/maps to list must have the form file(map)\n");
		ACTION1("Illegal specifier %s ignored\n",file);
		return False;
	    }
	    *(map-1)='\0';
	    *tmp= '\0';
	}
#ifdef DEBUG
	if (warningLevel>9)
	    INFO2("Adding %s(%s) to listings\n",file,(map?map:"*"));
#endif
    }
    if (nListed>=szListing) {
	if (szListing<1)	szListing= 10;
	else		szListing*= 2;
	filesToList= uTypedRealloc(filesToList,szListing,char *);
	mapsToList= uTypedRealloc(mapsToList,szListing,char *);
	if ((!filesToList) || (!mapsToList)) {
	    WSGO("Couldn't allocate list of files and maps\n");
	    ACTION("Exiting\n");
	    exit(1);
	}
    }
    filesToList[nListed]= file;
    mapsToList[nListed]= map;
    nListed++;
    if (file!=NULL)
	nFilesListed++;
    return True;
}

/***====================================================================***/

#define	M(m)	fprintf(stderr,(m))
#define	M1(m,a)	fprintf(stderr,(m),(a))

void
#if NeedFunctionPrototypes
Usage(int argc,char *argv[])
#else
Usage(argc,argv)
    int 	argc;
    char *	argv[];
#endif
{
    if (!xkblist)
	 M1("Usage: %s [options] input-file [ output-file ]\n",argv[0]);
    else M1("Usage: %s [options] file[(map)] ...\n",argv[0]);
    M("Legal options:\n");
    M("-?,-help             Print this message\n");
    if (!xkblist) {
	M("-a                   Show all actions\n");
	M("-C                   Create a C header file\n");
    }
#ifdef DEBUG
    M("-d [flags]           Report debugging information\n");
#endif
    M("-em1 <msg>           Print <msg> before printing first error message\n");
    M("-emp <msg>           Print <msg> at the start of each message line\n");
    M("-eml <msg>           If there were any errors, print <msg> before exiting\n");
    if (!xkblist) {
	M("-dflts               Compute defaults for missing parts\n");
	M("-I[<dir>]            Specifies a top level directory for include\n");
	M("                     directives. Multiple directories are legal.\n");
	M("-l[ist]              List matching maps in the specified files\n");
    }
    M("-m[ap] <map>         Specifies map to compile\n");
    if (!xkblist)
	M("-merge               Merge file with map on server\n");
    M("-o <file>            Specifies output file name\n");
    if (!xkblist) {
	M("-opt[ional] <parts>  Specifies optional components of keymap\n");
	M("                     Errors in optional parts are not fatal\n");
	M("                     <parts> can be any combination of:\n");
	M("                     c: compat map         g: geometry\n");
	M("                     k: keycodes           s: symbols\n");
	M("                     t: types\n");
    }
    M("-R[<DIR>]            Specifies the root directory for\n");
    M("                     relative path names\n");
    M("-synch               Force synchronization\n");
    if (xkblist) {
	M("-v [<flags>]         Set level of detail for listing:\n");
	M("                     f: list fully specified names\n");
	M("                     h: also list hidden maps\n");
	M("                     l: long listing (show flags)\n");
	M("                     p: also list partial maps\n");
	M("                     default is all options off\n");
    }
    M("-w [<lvl>]           Set warning level (0=none, 10=all)\n");
    if (!xkblist) {
	M("-xkb                 Create an XKB source (.xkb) file\n");
	M("-xkm                 Create a compiled key map (.xkm) file\n");
    }
    return;
}

/***====================================================================***/

Bool
#if NeedFunctionPrototypes
parseArgs(int argc,char *argv[])
#else
parseArgs(argc,argv)
    int		argc;
    char *	argv[];
#endif
{
register int i,tmp;

    i= strlen(argv[0]);
    tmp= strlen("xkblist");
    if ((i>=tmp)&&(strcmp(&argv[0][i-tmp],"xkblist")==0)) {
	xkblist= True;
    }
    for (i=1;i<argc;i++) {
	if ((argv[i][0]!='-')||(uStringEqual(argv[i],"-"))) {
	    if (!xkblist) {
		if (inputFile==NULL)
		    inputFile= argv[i];
		else if (outputFile==NULL)
		    outputFile= argv[i];
		else if (warningLevel>0) {
		    WARN("Too many file names on command line\n");
		    ACTION3("Compiling %s, writing to %s, ignoring %s\n",
					inputFile,outputFile,argv[i]);
		}
	    }
	    else if (!AddListing(argv[i],NULL))
		return False;
	}
	else if ((strcmp(argv[i],"-?")==0)||(strcmp(argv[i],"-help")==0)) {
	    Usage(argc,argv);
	    exit(0);
	}
	else if ((strcmp(argv[i],"-a")==0)&&(!xkblist)) {
	    showImplicit= True;
	}
	else if ((strcmp(argv[i],"-C")==0)&&(!xkblist)) {
	    if ((outputFormat!=WANT_DEFAULT)&&(outputFormat!=WANT_C_HDR)) {
		if (warningLevel>0) {
		    WARN("Multiple output file formats specified\n");
		    ACTION1("\"%s\" flag ignored\n",argv[i]);
		}
	    }
	    else outputFormat= WANT_C_HDR;
	}
#ifdef DEBUG
	else if (strcmp(argv[i],"-d")==0) {
	    if ((i>=(argc-1))||(!isdigit(argv[i+1][0]))) {
		debugFlags= 1;
	    }
	    else {
		sscanf(argv[++i],"%i",&debugFlags);
	    }
	    INFO1("Setting debug flags to %d\n",debugFlags);
	}
#endif
	else if ((strcmp(argv[i],"-dflts")==0)&&(!xkblist)) {
	    computeDflts= True;
	}
	else if (strcmp(argv[i],"-em1")==0) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No pre-error message specified\n");
		    ACTION("Trailing \"-em1\" option ignored\n");
		}
	    }
	    else if (preErrorMsg!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple pre-error messsages specified\n");
		    ACTION2("Compiling %s, ignoring %s\n",preErrorMsg,argv[i]);
	 	}
	    }
	    else preErrorMsg= argv[i];
	}
	else if (strcmp(argv[i],"-emp")==0) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No error prefix specified\n");
		    ACTION("Trailing \"-emp\" option ignored\n");
		}
	    }
	    else if (errorPrefix!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple error prefixes specified\n");
		    ACTION2("Compiling %s, ignoring %s\n",errorPrefix,argv[i]);
	 	}
	    }
	    else errorPrefix= argv[i];
	}
	else if (strcmp(argv[i],"-eml")==0) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No post-error message specified\n");
		    ACTION("Trailing \"-eml\" option ignored\n");
		}
	    }
	    else if (postErrorMsg!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple post-error messages specified\n");
		    ACTION2("Compiling %s, ignoring %s\n",postErrorMsg,argv[i]);
	 	}
	    }
	    else postErrorMsg= argv[i];
	}
	else if ((strncmp(argv[i],"-I",2)==0)&&(!xkblist)) {
	    if (!XkbAddDirectoryToPath(&argv[i][2])) {
		ACTION("Exiting\n");
	    }
	    exit(1);
	}
	else if (((strcmp(argv[i],"-l")==0)||(strcmp(argv[i],"-list")==0))&&
							(!xkblist)) {
	    if (outputFormat!=WANT_DEFAULT) {
		if (warningLevel>0) {
		    WARN("Multiple output file formats specified\n");
		    ACTION1("\"%s\" flag ignored\n",argv[i]);
		}
	    }
	    else {
		xkblist= True;
		if ((inputFile)&&(!AddListing(inputFile,NULL)))
		     return False;
		else inputFile= NULL;
		if ((outputFile)&&(!AddListing(outputFile,NULL)))
		     return False;
		else outputFile= NULL;
	    }
	}
	else if ((strcmp(argv[i],"-m")==0)||(strcmp(argv[i],"-map")==0)) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No map name specified\n");
		    ACTION1("Trailing \"%s\" option ignored\n",argv[i-1]);
		}
	    }
	    else if (xkblist) {
		 if (!AddListing(NULL,argv[i]))
		    return False;
	    }
	    else if (inputMap!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple map names specified\n");
		    ACTION2("Compiling %s, ignoring %s\n",inputMap,argv[i]);
	 	}
	    }
	    else inputMap= argv[i];
	}
	else if ((strcmp(argv[i],"-merge")==0)&&(!xkblist)) {
	    merge= True;
	}
	else if (strcmp(argv[i],"-o")==0) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No output file specified\n");
		    ACTION("Trailing \"-o\" option ignored\n");
		}
	    }
	    else if (outputFile!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple output files specified\n");
		    ACTION2("Compiling %s, ignoring %s\n",outputFile,argv[i]);
		}
	    }
	    else outputFile= argv[i];
	}
	else if (((strcmp(argv[i],"-opt")==0)||(strcmp(argv[i],"optional")==0))
		 						&&(!xkblist)) {
	    if (++i>=argc) {
		if (warningLevel>0) {
		    WARN("No optional components specified\n");
		    ACTION1("Trailing \"%s\" option ignored\n",argv[i-1]);
		}
	    }
	    else {
		char *tmp;
		for (tmp=argv[i];(*tmp!='\0');tmp++) {
		    switch (*tmp) {
			case 'c': case 'C':
			    optionalParts|= XkmCompatMapMask;
			    break;
			case 'g': case 'G':
			    optionalParts|= XkmGeometryMask;
			    break;
			case 'k': case 'K':
			    optionalParts|= XkmKeyNamesMask;
			    break;
			case 's': case 'S':
			    optionalParts|= XkmSymbolsMask;
			    break;
			case 't': case 'T':
			    optionalParts|= XkmTypesMask;
			    break;
			default:
			    if (warningLevel>0) {
				WARN1("Illegal component for %s option\n",
								argv[i-1]);
				ACTION1("Ignoring unknown specifier \"%c\"\n",
								(unsigned int)*tmp);
			    }
			    break;
		    }
		}
	    }
	}
	else if (strncmp(argv[i],"-R",2)==0) {
	    if (argv[i][2]=='\0') {
		if (warningLevel>0) {
		    WARN("No root directory specified\n");
		    ACTION("Ignoring -R option\n");
		}
	    }
	    else if (rootDir!=NULL) {
		if (warningLevel>0) {
		    WARN("Multiple root directories specified\n");
		    ACTION2("Using %s, ignoring %s\n",rootDir,argv[i]);
		}
	    }
	    else rootDir= &argv[i][2];
	}
	else if ((strcmp(argv[i],"-synch")==0)||(strcmp(argv[i],"-s")==0)) {
	    synch= True;
	}
	else if (strncmp(argv[i],"-v",2)==0) {
	    char *str;
	    if (argv[i][2]!='\0') 
		 str= &argv[i][2];
	    else if ((i<(argc-1))&&(argv[i+1][0]!='-'))
		 str= argv[++i];
	    else str= NULL;
	    if (str) {
		for (;*str;str++) {
		    switch (*str) {
			case 'f': verboseLevel|= WantFullNames; break;
			case 'h': verboseLevel|= WantHiddenMaps; break;
			case 'l': verboseLevel|= WantLongListing; break;
			case 'p': verboseLevel|= WantPartialMaps; break;
			default:
			    if (warningLevel>4) {
				WARN1("Unknown verbose option \"%c\"\n",(unsigned int)*str);
				ACTION("Ignored\n");
			    }
			    break;
		    }
		}
	    }
	}
	else if (strncmp(argv[i],"-w",2)==0) {
	    if ((i>=(argc-1))||(!isdigit(argv[i+1][0]))) {
		if (isdigit(argv[i][1]))
		     sscanf(&argv[i][1],"%i",&warningLevel);
		else warningLevel= 0;
	    }
	    else {
		sscanf(argv[++i],"%i",&warningLevel);
	    }
	}
	else if ((strcmp(argv[i],"-xkb")==0)&&(!xkblist)) {
	    if ((outputFormat!=WANT_DEFAULT)&&(outputFormat!=WANT_XKB_FILE)) {
		if (warningLevel>0) {
		    WARN("Multiple output file formats specified\n");
		    ACTION1("\"%s\" flag ignored\n",argv[i]);
		}
	    }
	    else outputFormat= WANT_XKB_FILE;
	}
	else if ((strcmp(argv[i],"-xkm")==0)&&(!xkblist)) {
	    if ((outputFormat!=WANT_DEFAULT)&&(outputFormat!=WANT_XKM_FILE)) {
		if (warningLevel>0) {
		    WARN("Multiple output file formats specified\n");
		    ACTION1("\"%s\" flag ignored\n",argv[i]);
		}
	    }
	    else outputFormat= WANT_XKM_FILE;
	}
	else {
	    ERROR1("Unknown flag \"%s\" on command line\n",argv[i]);
	    Usage(argc,argv);
	    return False;
	}
    }
    if (rootDir) {
	if (warningLevel>8) {
	    WARN1("Changing root directory to \"%s\"\n",rootDir);
	}
	if ((chdir(rootDir)<0) && (warningLevel>0)) {
	    WARN1("Couldn't change root directory to \"%s\"\n",rootDir);
	    ACTION("Root directory (-R) option ignored\n");
	}
    }
    if (xkblist)
	inputFormat= INPUT_XKB;
    else if (inputFile==NULL) {
	ERROR("No input file specified\n");
	return False;
    }
    else if (uStringEqual(inputFile,"-")) {
	inputFormat= INPUT_XKB;
    }
    else if (strchr(inputFile,':')==0) {
	int	len;
	len= strlen(inputFile);
	if (inputFile[len-1]==')') {
	    char *tmp;
	    if ((tmp=strchr(inputFile,'('))!=0) {
		*tmp= '\0';  inputFile[len-1]= '\0';
		tmp++;
		if (*tmp=='\0') {
		     WARN("Empty map in filename\n");
		     ACTION("Ignored\n");
		}
		else if (inputMap==NULL) {
		    inputMap= uStringDup(tmp);
		}
		else {
		    WARN("Map specified in filename and with -m flag\n");
		    ACTION1("map from name (\"%s\") ignored\n",tmp);
		}
	    }
	    else {
		ERROR1("Illegal name \"%s\" for input file\n",inputFile);
		return False;
	    }
	}
	if ((len>4)&&(strcmp(&inputFile[len-4],".xkm")==0)) {
	    inputFormat= INPUT_XKM;
	}
	else {
	    FILE *file;
	    file= fopen(inputFile,"r");
	    if (file) {
		if (XkmProbe(file))	inputFormat= INPUT_XKM;
		else			inputFormat= INPUT_XKB;
		fclose(file);
	    }
	    else {
		fprintf(stderr,"Cannot open \"%s\" for reading\n",inputFile);
		return False;
	    }
	}
    }
    else {
	inDpyName= inputFile;
	inputFile= NULL;
	inputFormat= INPUT_XKM;
    }

    if (outputFormat==WANT_DEFAULT) {
	if (xkblist)				outputFormat= WANT_LISTING;
	else if (inputFormat==INPUT_XKB)	outputFormat= WANT_XKM_FILE;
	else					outputFormat= WANT_XKB_FILE;
    }
    if ((outputFormat==WANT_LISTING)&&(inputFormat!=INPUT_XKB)) {
	if (inputFile)
	     ERROR("Cannot generate a listing from a .xkm file (yet)\n");
	else ERROR("Cannot generate a listing from an X connection (yet)\n");
	return False;
    }
    if (xkblist) {
	if (outputFile==NULL)	outputFile= uStringDup("-");
	else if (strchr(outputFile,':')!=NULL) {
	    ERROR("Cannot write a listing to an X connection\n");
	    return False;
	}
    }
    else if ((!outputFile) && (inputFile) && uStringEqual(inputFile,"-")) {
	int len= strlen("stdin")+strlen(fileTypeExt[outputFormat])+2;
	outputFile= uTypedCalloc(len,char);
	if (outputFile==NULL) {
	    WSGO("Cannot allocate space for output file name\n");
	    ACTION("Exiting\n");
	    exit(1);
	}
	sprintf(outputFile,"stdin.%s",fileTypeExt[outputFormat]);
    }
    else if ((outputFile==NULL)&&(inputFile!=NULL)) {
	int len;
	char *base,*ext;

	if (inputMap==NULL)  {
	    base= strrchr(inputFile,'/');
	    if (base==NULL)	base= inputFile;
	    else		base++;
	}
	else base= inputMap;

	len= strlen(base)+strlen(fileTypeExt[outputFormat])+2;
	outputFile= uTypedCalloc(len,char);
	if (outputFile==NULL) {
	    WSGO("Cannot allocate space for output file name\n");
	    ACTION("Exiting\n");
	    exit(1);
	}
	ext= strrchr(base,'.');
	if (ext==NULL)
	    sprintf(outputFile,"%s.%s",base,fileTypeExt[outputFormat]);
	else {
	    strcpy(outputFile,base);
	    strcpy(&outputFile[ext-base+1],fileTypeExt[outputFormat]);
	}
    }
    else if (outputFile==NULL) {
	int len;
	char *ch,*name,buf[128];
	if (inDpyName[0]==':')	
	     sprintf(name=buf,"server%s",inDpyName);
	else name= inDpyName;

	len= strlen(name)+strlen(fileTypeExt[outputFormat])+2;
	outputFile= uTypedCalloc(len,char);
	if (outputFile==NULL) {
	    WSGO("Cannot allocate space for output file name\n");
	    ACTION("Exiting\n");
	    exit(1);
	}
	strcpy(outputFile,name);
	for (ch=outputFile;(*ch)!='\0';ch++) {
	    if 	(*ch==':')	*ch= '-';
	    else if (*ch=='.')	*ch= '_';
	}
	*ch++= '.';
	strcpy(ch,fileTypeExt[outputFormat]);
    }
    else if (strchr(outputFile,':')!=NULL) {
	outDpyName= outputFile;
	outputFile= NULL;
	outputFormat=  WANT_X_SERVER;
    }
    return True;
}

Display *
#if NeedFunctionPrototypes
GetDisplay(char *program,char *dpyName)
#else
GetDisplay(program,dpyName)
    char *	program;
    char *	dpyName;
#endif
{
int	mjr,mnr,error;
Display	*dpy;

    mjr= XkbMajorVersion;
    mnr= XkbMinorVersion;
    dpy= XkbOpenDisplay(dpyName,NULL,NULL,&mjr,&mnr,&error);
    if (dpy==NULL) {
	switch (error) {
	    case XkbOD_BadLibraryVersion:
		INFO3("%s was compiled with XKB version %d.%02d\n",
				program,XkbMajorVersion,XkbMinorVersion);
		ERROR2("X library supports incompatible version %d.%02d\n",
				mjr,mnr);
		break;
	    case XkbOD_ConnectionRefused:
		ERROR1("Cannot open display \"%s\"\n",dpyName);
		break;
	    case XkbOD_NonXkbServer:
		ERROR1("XKB extension not present on %s\n",dpyName);
		break;
	    case XkbOD_BadServerVersion:
		INFO3("%s was compiled with XKB version %d.%02d\n",
				program,XkbMajorVersion,XkbMinorVersion);
		ERROR3("Server %s uses incompatible version %d.%02d\n",
				dpyName,mjr,mnr);
		break;
	    default:
		WSGO1("Unknown error %d from XkbOpenDisplay\n",error);
	}
    }
    else if (synch)
	XSynchronize(dpy,True);
    return dpy;
}

/***====================================================================***/

static void
#if NeedFunctionPrototypes
ListFile(char *fileName,XkbFile *map)
#else
ListFile(fileName,map)
char *		fileName;
XkbFile *	map;
#endif
{
register unsigned	flags;
char *			mapName;

    flags= map->flags;
    if ((flags&XkbLC_Hidden)&&(!(verboseLevel&WantHiddenMaps)))
	return;
    if ((flags&XkbLC_Partial)&&(!(verboseLevel&WantPartialMaps)))
	return;
    if (verboseLevel&WantLongListing) {
	printf((flags&XkbLC_Hidden)?"h":"-");
	printf((flags&XkbLC_Default)?"d":"-");
	printf((flags&XkbLC_Partial)?"p":"-");
	printf("----- ");
	if (map->type==XkmSymbolsIndex) {
	    printf((flags&XkbLC_AlphanumericKeys)?"a":"-");
	    printf((flags&XkbLC_ModifierKeys)?"m":"-");
	    printf((flags&XkbLC_KeypadKeys)?"k":"-");
	    printf((flags&XkbLC_FunctionKeys)?"f":"-");
	    printf((flags&XkbLC_AlternateGroup)?"g":"-");
	    printf("--- ");
	}
        else printf("-------- ");
    }
    mapName= map->name;
    if ((!(verboseLevel&WantFullNames))&&((flags&XkbLC_Default)!=0))
	mapName= NULL;
    if (mapName)
	 printf("%s(%s)\n",fileName,mapName);
    else printf("%s\n",fileName);
    return;
}

static int
#if NeedFunctionPrototypes
GenerateListing(void)
#else
GenerateListing()
#endif
{
int		i;
FILE *		inputFile;
XkbFile *	rtrn,*mapToUse;
unsigned	oldWarningLevel;

    if (nFilesListed<1) {
	ERROR("Must specify at least one file or pattern to list\n");
	return 1;
    }
#ifdef DEBUG
    if (warningLevel>9)
	fprintf(stderr,"should list:\n");
#endif
    for (i=0;i<nListed;i++) {
#ifdef DEBUG
	if (warningLevel>9) {
	    fprintf(stderr,"%s(%s)\n",(filesToList[i]?filesToList[i]:"*"),
					(mapsToList[i]?mapsToList[i]:"*"));
	}
#endif
	oldWarningLevel= warningLevel;
	warningLevel= 0;
	if (filesToList[i]) {
	    inputFile= fopen(filesToList[i],"r");
	    if (!inputFile) {
		if (oldWarningLevel>5)
		    WARN1("Couldn't open \"%s\"\n",filesToList[i]);
		continue;
	    }
	    if (XKBParseFile(inputFile,&rtrn)&&(rtrn!=NULL)) {
		mapToUse= rtrn;
		for (;mapToUse;mapToUse= (XkbFile *)mapToUse->common.next) {
		    if (mapsToList[i]!=NULL) {
			fprintf(stderr,"Don't know how to pattern match yet\n");
			continue;
		    }
		    ListFile(filesToList[i],mapToUse);
		}
	    }
	    fclose(inputFile);
	}
	warningLevel= oldWarningLevel;
    }
    return 1;
}

/***====================================================================***/

extern int yydebug;

int
#if NeedFunctionPrototypes
main(int argc,char *argv[])
#else
main(argc,argv)
    int		argc;
    char *	argv[];
#endif
{
FILE 	*	file;
XkbFile	*	rtrn;
XkbFile	*	mapToUse;
int		ok;
XkbFileInfo 	result;
Status		status;

#ifdef Lynx
{
    extern FILE *yyin;
    yyin = stdin;
    uSetEntryFile(NullString);
    uSetDebugFile(NullString);
    uSetErrorFile(NullString);
}
#endif
    if (!parseArgs(argc,argv))
	exit(1);
#ifdef DEBUG
    if (debugFlags&0x2)
	yydebug= 1;
#ifdef sgi
    if (debugFlags&0x4)
	mallopt(M_DEBUG,1);
#endif
#endif
    if (preErrorMsg)
	uSetPreErrorMessage(preErrorMsg);
    if (errorPrefix)
	uSetErrorPrefix(errorPrefix);
    if (postErrorMsg)
	uSetPostErrorMessage(postErrorMsg);
    file= NULL;
    XkbInitAtoms(NULL);
    XkbInitIncludePath();
    if (xkblist) {
	Bool	gotSome;
	gotSome= GenerateListing();
	if ((warningLevel>7)&&(!gotSome))
	    return -1;
	return 0;
    }
    if (inputFile!=NULL) {
	if (uStringEqual(inputFile,"-")) {
	    static char *in= "stdin";
	    file= stdin;
	    inputFile= in;
	}
	else {
	    file= fopen(inputFile,"r");
	}
    }
    else if (inDpyName!=NULL) {
	inDpy= GetDisplay(argv[0],inDpyName);
	if (!inDpy) {
	    ACTION("Exiting\n");
	    exit(1);
	}
    }
    if (outDpyName!=NULL) {
	outDpy= GetDisplay(argv[0],outDpyName);
	if (!outDpy) {
	    ACTION("Exiting\n");
	    exit(1);
	}
    }
    if ((inDpy==NULL) && (outDpy==NULL)) {
	int	mjr,mnr;
	mjr= XkbMajorVersion;
	mnr= XkbMinorVersion;
	if (!XkbLibraryVersion(&mjr,&mnr)) {
	    INFO3("%s was compiled with XKB version %d.%02d\n",
				argv[0],XkbMajorVersion,XkbMinorVersion);
	    ERROR2("X library supports incompatible version %d.%02d\n",
				mjr,mnr);
	    ACTION("Exiting\n");
	    exit(1);
	}
    }
    if (file) {
	ok= True;
	setScanState(inputFile,1);
	if ((inputFormat==INPUT_XKB)&&(XKBParseFile(file,&rtrn)&&(rtrn!=NULL))){
	    fclose(file);
	    mapToUse= rtrn;
	    if (inputMap!=NULL) {
		while ((mapToUse)&&(!uStringEqual(mapToUse->name,inputMap))) {
		    mapToUse= (XkbFile *)mapToUse->common.next;
		}
		if (!mapToUse) {
		    FATAL2("No map named \"%s\" in \"%s\"\n",inputMap,
		    						inputFile);
		    /* NOTREACHED */
		}
	    }
	    else if (rtrn->common.next!=NULL) {
		mapToUse= rtrn;
		for (;mapToUse;mapToUse= (XkbFile*)mapToUse->common.next) {
		    if (mapToUse->flags&XkbLC_Default)
			break;
		}
		if (!mapToUse) {
		    mapToUse= rtrn;
		    if (warningLevel>4) {
			WARN1("No map specified, but \"%s\" has several\n",
								inputFile);
			ACTION1("Using the first defined map, \"%s\"\n",
								mapToUse->name);
		    }
		}
	    }
	    bzero((char *)&result,sizeof(result));
	    result.type= mapToUse->type;
	    if ((result.xkb= XkbAllocKeyboard())==NULL) {
		WSGO("Cannot allocate keyboard description\n");
		/* NOTREACHED */
	    }
	    switch (mapToUse->type) {
		case XkmSemanticsFile:
		case XkmLayoutFile:
		case XkmKeymapFile:
		    ok= CompileKeymap(mapToUse,&result,MergeReplace);
		    break;
		case XkmKeyNamesIndex:
		    ok= CompileKeycodes(mapToUse,&result,MergeReplace);
		    break;
		case XkmTypesIndex:
		    ok= CompileKeyTypes(mapToUse,&result,MergeReplace);
		    break;
		case XkmSymbolsIndex:
		    ERROR("Symbols files cannot be compiled on their own\n");
		    ACTION("You must include them in other files\n");
		    ok= False;
		    break;
		case XkmCompatMapIndex:
		    ok= CompileCompatMap(mapToUse,&result,MergeReplace,NULL);
		    break;
		case XkmGeometryFile:
		case XkmGeometryIndex:
		    /* if it's just a geometry, invent key names */
		    result.xkb->flags|= AutoKeyNames;
		    ok= CompileGeometry(mapToUse,&result,MergeReplace);
		    break;
		default:
		    WSGO1("Unknown file type %d\n",mapToUse->type);
		    ok= False;
		    break;
	    }
	}
	else if (inputFormat==INPUT_XKM) {
	    unsigned tmp;
	    bzero((char *)&result,sizeof(result));
	    if ((result.xkb= XkbAllocKeyboard())==NULL) {
		WSGO("Cannot allocate keyboard description\n");
		/* NOTREACHED */
	    }
	    tmp= XkmReadFile(file,0,XkmKeymapLegal,&result);
	    if (tmp==XkmKeymapLegal) {
		ERROR1("Cannot read XKM file \"%s\"\n",inputFile);
		ok= False;
	    }
	}
	else {
	    INFO1("Errors encountered in %s; not compiled.\n",inputFile);
	    ok= False;
	}
    }
    else if (inDpy!=NULL) {
	bzero((char *)&result,sizeof(result));
	result.type= XkmKeymapFile;
	result.xkb= XkbGetMap(inDpy,XkbAllMapComponentsMask,XkbUseCoreKbd);
	if (result.xkb==NULL)
	    WSGO("Cannot load keyboard description\n");
	if (XkbGetIndicatorMap(inDpy,~0,result.xkb)!=Success)
	    WSGO("Could not load indicator map\n");
	if (XkbGetControls(inDpy,XkbAllControlsMask,result.xkb)!=Success)
	    WSGO("Could not load keyboard controls\n");
	if (XkbGetCompatMap(inDpy,XkbAllCompatMask,result.xkb)!=Success)
	    WSGO("Could not load compatibility map\n");
	if (XkbGetNames(inDpy,XkbAllNamesMask,result.xkb)!=Success)
	    WSGO("Could not load names\n");
	if ((status=XkbGetGeometry(inDpy,result.xkb))!=Success) {
	    if (warningLevel>3) {
		char buf[100];
		buf[0]= '\0';
		XGetErrorText(inDpy,status,buf,100);
		WARN1("Could not load keyboard geometry for %s\n",inDpyName);
		ACTION1("%s\n",buf);
		ACTION("Resulting keymap file will not describe geometry\n");
	    }
	}
	if (computeDflts)
	     ok= (ComputeKbdDefaults(result.xkb)==Success);
	else ok= True;
    }
    else {
	fprintf(stderr,"Cannot open \"%s\" to compile\n",inputFile);
	ok= 0;
    }
    if (ok) {
	FILE *out;
	if ((inDpy!=outDpy)&&
	    (XkbChangeKbdDisplay(outDpy,&result)!=Success)) {
	    WSGO2("Error converting keyboard display from %s to %s\n",
	    						inDpyName,outDpyName);
	    exit(1);
	}
	if (outputFile!=NULL) {
	    if (uStringEqual(outputFile,"-")) {
		static char *of= "stdout";
		out= stdout;
		outputFile= of;
	    }
	    else {
		out= fopen(outputFile,"w");
		if (out==NULL) {
		    ERROR1("Cannot open \"%s\" to write keyboard description\n",
								outputFile);
		     ACTION("Exiting\n");
		     exit(1);
		}
	    }
	}
	switch (outputFormat) {
	    case WANT_XKM_FILE:
		ok= XkbWriteXKMFile(out,&result);
		break;
	    case WANT_XKB_FILE:
		ok= XkbWriteXKBFile(out,&result,showImplicit,NULL,NULL);
		break;
	    case WANT_C_HDR:
		ok= XkbWriteCFile(out,outputFile,&result);
		break;
	    case WANT_X_SERVER:
	    	if (!(ok= XkbWriteToServer(&result))) {
		    ERROR2("%s in %s\n",_XkbErrMessages[_XkbErrCode],
				_XkbErrLocation?_XkbErrLocation:"unknown");
		    ACTION1("Couldn't write keyboard description to %s\n",
								outDpyName);
		}
		break;
	    default:
		WSGO1("Unknown output format %d\n",outputFormat);
		ACTION("No output file created\n");
		ok= False;
		break;
	}
	if (outputFormat!=WANT_X_SERVER) {
	    fclose(out);
	    if (!ok) {
		ERROR2("%s in %s\n",_XkbErrMessages[_XkbErrCode],
				_XkbErrLocation?_XkbErrLocation:"unknown");
		ACTION1("Output file \"%s\" removed\n",outputFile);
		unlink(outputFile);
	    }
	}
    }
    if (inDpy) 
	XCloseDisplay(inDpy);
    inDpy= NULL;
    if (outDpy)
	XCloseDisplay(outDpy);
    uFinishUp();
    return (ok==0);
}
