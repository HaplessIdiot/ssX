/*
 * $XConsortium: xf86Config.c,v 1.2 94/03/28 21:22:51 dpw Exp $
 * $XFree86$
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86_OSlib.h"

#include "xf86Procs.h"

#ifndef SERVER_CONFIG_FILE
#define SERVER_CONFIG_FILE "/usr/XF86/Xconfig"
#endif

#define INIT_CONFIG
#include "xf86_Config.h"

#define CONFIG_BUF_LEN     1024

static FILE * configFile   = NULL;
static int    configStart  = 0;           /* start of the current token */
static int    configPos    = 0;           /* current readers position */
static int    configLineNo = 0;           /* linenumber */
static char   *configBuf,*configRBuf;     /* buffer for lines */
static char   *configPath;                /* path to config file */
static char   *fontPath = NULL;           /* font path */
static int    pushToken = LOCK_TOKEN;
static LexRec val;                        /* global return value */

static DisplayModePtr pModes = NULL;

static int screenno = -100;      /* some little number ... */

static int textClockValue = -1;

static int tol_table[] = {0, 5, 10, 50, 100, 500, 1000, 2000};
#define NUM_PASSES (sizeof(tol_table)/sizeof(tol_table[0]))

extern char *getenv();
extern char *defaultFontPath;
extern char *rgbPath;

extern Bool xf86fpFlag, xf86coFlag;

extern int defaultColorVisualClass;

#define DIR_FILE	"/fonts.dir"

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
static char *
xf86GetPathElem(pnt)
     char **pnt;
{
  char *p1;

  p1 = *pnt;
  *pnt = index(*pnt, ',');
  if (*pnt != NULL) {
    **pnt = '\0';
    *pnt += 1;
  }
  return(p1);
}

/*
 * StrToUL --
 *
 *	A portable, but restricted, version of strtoul().  It only understands
 *	hex, octal, and decimal.  But it's good enough for our needs.
 */
unsigned int StrToUL(str)
char *str;
{
  int base = 10;
  char *p = str;
  unsigned int tot = 0;

  if (*p == '0') {
    p++;
    if (*p == 'x') {
      p++;
      base = 16;
    }
    else
      base = 8;
  }
  while (*p) {
    if ((*p >= '0') && (*p <= ((base == 8)?'7':'9'))) {
      tot = tot * base + (*p - '0');
    }
    else if ((base == 16) && (*p >= 'a') && (*p <= 'f')) {
      tot = tot * base + 10 + (*p - 'a');
    }
    else if ((base == 16) && (*p >= 'A') && (*p <= 'F')) {
      tot = tot * base + 10 + (*p - 'A');
    }
    else {
      return(tot);
    }
    p++;
  }
  return(tot);
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */
#define CHECK_TYPE(mode, type) ((S_IFMT & (mode)) == (type))
static char *
xf86ValidateFontPath(path)
     char *path;
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1;
  struct stat stat_buf;
  int flag;

  tmp_path = (char *)Xcalloc(strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = xf86GetPathElem(&next);
    if (*path_elem == '/') {
      flag = stat(path_elem, &stat_buf);
      if (flag == 0)
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFDIR))
	  flag = -1;
      if (flag != 0) {
        ErrorF("Warning: The directory \"%s\" does not exist.\n", path_elem);
	ErrorF("         Entry deleted from font path.\n");
	continue;
      }
      else {
	p1 = (char *)xalloc(strlen(path_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, path_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
	    flag = -1;
	xfree(p1);
	if (flag != 0) {
	  ErrorF("Warning: 'fonts.dir' not found (or not valid) in \"%s\".\n", 
		 path_elem);
	  ErrorF("          Entry deleted from font path.\n");
	  ErrorF("          (Run 'mkfontdir' on \"%s\").\n", path_elem);
	  continue;
	}
      }
    }

    /*
     * Either an OK directory, or a font server name.  So add it to
     * the path.
     */
    if (out_pnt != tmp_path)
      *out_pnt++ = ',';
    strcat(out_pnt, path_elem);
    out_pnt += strlen(path_elem);
  }
  return(tmp_path);
}

/*
 * getToken --
 *      Read next Token form the config file. Handle the global variable
 *      pushToken.
 */
static int
getToken(tab)
     SymTabRec tab[];
{
  int          c, i;

  /*
   * First check whether pushToken has a different value than LOCK_TOKEN.
   * In this case rBuf[] contains a valid STRING/TOKEN/NUMBER. But in the other
   * case the next token must be read from the input.
   */
  if (pushToken == EOF) return(EOF);
  else if (pushToken == LOCK_TOKEN)
    {
      
      c = configBuf[configPos];
      
      /*
       * Get start of next Token. EOF is handled, whitespaces & comments are
       * skipped. 
       */
      do {
	if (!c)  {
	  if (fgets(configBuf,CONFIG_BUF_LEN-1,configFile) == NULL)
	    {
	      return( pushToken = EOF );
	    }
	  configLineNo++;
	  configStart = configPos = 0;
	}
	while (((c=configBuf[configPos++])==' ') || ( c=='\t') || ( c=='\n'));
	if (c == '#') c = '\0'; 
      } while (!c);
      configStart = configPos;
      
      /*
       * Numbers are returned immediately ...
       */
      if (isdigit(c))
	{
          extern double atof();
	  int base;

	  if (c == '0')
	    if ((configBuf[configPos] == 'x') || 
		(configBuf[configPos] == 'X'))
	      base = 16;
	    else
	      base = 8;
	  else
	    base = 10;

	  configRBuf[0] = c; i = 1;
	  while (isdigit(c = configBuf[configPos++]) || 
		 (c == '.') || (c == 'x') || 
		 ((base == 16) && (((c >= 'a') && (c <= 'f')) ||
				   ((c >= 'A') && (c <= 'F')))))
            configRBuf[i++] = c;
	  configRBuf[i] = '\0';
	  val.num = StrToUL(configRBuf);
          val.realnum = atof(configRBuf);
	  return(NUMBER);
	}
      
      /*
       * All Strings START with a \" ...
       */
      else if (c == '\"')
	{
	  i = -1;
	  do {
	    configRBuf[++i] = (c = configBuf[configPos++]);
	  } while ((c != '\"') && (c != '\n') && (c != '\0'));
	  configRBuf[i] = '\0';
	  val.str = (char *)xalloc(strlen(configRBuf) + 1);
	  strcpy(val.str, configRBuf);      /* private copy ! */
	  return(STRING);
	}
      
      /*
       * ... and now we MUST have a valid token. Since all tokens are handled
       * caseinsenitive, they are all lowercased internally. The search is
       * handled later along with the pushed tokens.
       */
      else
	{
	  configRBuf[0] = isupper (c) ? tolower(c) : c;
	  i = 0;
	  do {
	    c = configBuf[configPos++];
	    if (isupper (c))
	      c = tolower (c);
	    configRBuf[++i] = c;
	  } while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\0'));
	  configRBuf[i] = '\0'; i=0;
	}
      
    }
  else
    {
    
      /*
       * Here we deal with pushed tokens. Reinitialize pushToken again. If
       * the pushed token was NUMBER || STRING return them again ...
       */
      int temp = pushToken;
      pushToken = LOCK_TOKEN;
    
      if (temp == NUMBER || temp == STRING) return(temp);
    }
  
  /*
   * Joop, at last we have to lookup the token ...
   */
  if (tab)
    {
      i = 0;
      while (tab[i].token != -1)
	if (strcmp(configRBuf,tab[i].name) == 0)
	  return(tab[i].token);
	else
	  i++;
    }
  
  return(ERROR_TOKEN);       /* Error catcher */
}

/*
 * getScreenIndex --
 *	Given the screen token, returns the index in xf86Screens, or -1 if
 *	the screen type is not applicable to this server.
 */
static int
getScreenIndex(token)
     int token;
{
  int i;

  for (i = 0; xf86ScreenNames[i] >= 0 && xf86ScreenNames[i] != token; i++)
    ;
  if (xf86ScreenNames[i] < 0)
    return(-1);
  else
    return(i);
}

/*
 * validateGraphicsToken --
 *	If token is a graphics token, check it is in the list of validTokens
 */
static Bool
validateGraphicsToken(validTokens, token)
     int *validTokens;
     int token;
{
  int i;

  for (i = 0; GraphicsTab[i].token >= 0 && GraphicsTab[i].token != token; i++)
    ;
  if (GraphicsTab[i].token < 0)
    return(TRUE);        /* Not a graphics token */

  for (i = 0; validTokens[i] >= 0 && validTokens[i] != token; i++)
    ;
  return(validTokens[i] >= 0);
}

/*
 * tokenToString --
 *	returns the string corresponding to token
 */
static char *
tokenToString(table, token)
     SymTabPtr table;
     int token;
{
  int i;

  for (i = 0; table[i].token >= 0 && table[i].token != token; i++)
    ;
  if (table[i].token < 0)
    return("unknown");
  else
    return(table[i].name);
}
 
/*
 * configError --
 *      Print a READABLE ErrorMessage!!!  All information that is 
 *      interesting is printed.  Even a pointer to the erroneous place is
 *      printed.  Maybe our e-mail will be less :-)
 */
static void
configError(msg)
     char *msg;
{
  int i,j;

  ErrorF( "\nConfig Error: %s:%d\n\n%s", configPath, configLineNo, configBuf);
  for (i = 1, j = 1; i < configStart; i++, j++) 
    if (configBuf[i-1] != '\t')
      ErrorF(" ");
    else
      do
	ErrorF(" ");
      while (((j++)%8) != 0);
  for (i = configStart; i <= configPos; i++) ErrorF("^");
  ErrorF("\n%s\n", msg);
  exit(-1);                 /* simple exit ... */
}

/*
 * configKeyboard --
 *      Configure all keyboard related parameters
 */
static void
configKeyboard()
{
  int token, ntoken;

  xf86Info.dontZap       = FALSE;
  xf86Info.serverNumLock = FALSE;
  xf86Info.xleds         = 0L;
  xf86Info.kbdDelay      = 500;
  xf86Info.kbdRate       = 30;
  xf86Info.vtinit        = NULL;
  xf86Info.vtSysreq      = VT_SYSREQ_DEFAULT;
  xf86Info.specialKeyMap = (int *)xalloc((RIGHTCTL - LEFTALT + 1) *
                                            sizeof(int));
  xf86Info.specialKeyMap[LEFTALT - LEFTALT] = K_META;
  xf86Info.specialKeyMap[RIGHTALT - LEFTALT] = K_META;
  xf86Info.specialKeyMap[SCROLLLOCK - LEFTALT] = K_COMPOSE;
  xf86Info.specialKeyMap[RIGHTCTL - LEFTALT] = K_CONTROL;

  for (;;) {
    
    switch (token = getToken(KeyboardTab)) {

    case AUTOREPEAT:
      if (getToken(NULL) != NUMBER) configError("Autorepeat delay expected");
      xf86Info.kbdDelay = val.num;
      if (getToken(NULL) != NUMBER) configError("Autorepeat rate expected");
      xf86Info.kbdRate = val.num;
      break;

    case DONTZAP:
      xf86Info.dontZap = TRUE;
      break;

    case SERVERNUM:
      xf86Info.serverNumLock = TRUE;
      break;

    case XLEDS:
      while ((token= getToken(NULL)) == NUMBER)
	xf86Info.xleds |= 1L << (val.num-1);
      pushToken = token;
      break;

    case VTINIT:
      if (getToken(NULL) != STRING) configError("VTInit string expected");
      xf86Info.vtinit = val.str;
      if (xf86Verbose)
        ErrorF("%s VTInit: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case LEFTALT:
    case RIGHTALT:
    case SCROLLLOCK:
    case RIGHTCTL:
      ntoken = getToken(KeyMapTab);
      if ((ntoken == EOF) || (ntoken == STRING) || (ntoken == NUMBER)) 
	configError("KeyMap type token expected");
      else {
	switch(ntoken) {
	case K_META:
	case K_COMPOSE:
	case K_MODESHIFT:
	case K_MODELOCK:
	case K_SCROLLLOCK:
	case K_CONTROL:
          xf86Info.specialKeyMap[token - LEFTALT] = ntoken;
	  break;
	default:
	  configError("Illegal KeyMap type");
	  break;
	}
      }
      break;
    case VTSYSREQ:
#ifdef USE_VT_SYSREQ
      xf86Info.vtSysreq = TRUE;
      if (xf86Verbose && !VT_SYSREQ_DEFAULT)
        ErrorF("%s VTSysReq enabled\n", XCONFIG_GIVEN);
#else
      configError("VTSysReq not supported on this OS");
#endif
      break;

    default:
      pushToken = token;
      return;
    }
  }
}

/*
 * configMouse --
 *      Configure all mouse related parameters
 */
static void
configMouse()
{
  int token;

  xf86Info.baudRate        = 1200;
  xf86Info.sampleRate      = 0;
  xf86Info.emulate3Buttons = FALSE;
  xf86Info.chordMiddle     = FALSE;
  xf86Info.mouseFlags = 0;

  for (;;) {

    switch (token = getToken(MouseTab)) {

    case BAUDRATE:
      if (getToken(NULL) != NUMBER) configError("Baudrate expected");
      if (xf86Info.mseType + MICROSOFT == LOGIMAN)
	{
	  /* Moan if illegal baud rate!  [CHRIS-211092] */
	  if ((val.num != 1200) && (val.num != 9600))
	    configError("Only 1200 or 9600 Baud are supported by MouseMan");
	}
      xf86Info.baudRate = val.num;
      break;

    case SAMPLERATE:
      if (getToken(NULL) != NUMBER) configError("Sample rate expected");
      if (xf86Info.mseType + MICROSOFT == LOGIMAN)
	{
	  /* Moan about illegal sample rate!  [CHRIS-211092] */
	  configError("Selection of sample rate is not supported by MouseMan");
	}
      xf86Info.sampleRate = val.num;
      break;

    case EMULATE3:
      if (xf86Info.chordMiddle)
        configError("Can't use Emulate3Buttons with ChordMiddle");
      xf86Info.emulate3Buttons = TRUE;
      break;

    case CHORDMIDDLE:
      if (xf86Info.mseType + MICROSOFT == MICROSOFT ||
          xf86Info.mseType + MICROSOFT == LOGIMAN)
      {
        if (xf86Info.emulate3Buttons)
          configError("Can't use ChordMiddle with Emulate3Buttons");
        xf86Info.chordMiddle = TRUE;
      }
      else
        configError("ChordMiddle is only supported for MicroSoft and Logiman");
      break;

    case CLEARDTR:
#ifdef CLEARDTR_SUPPORT
      if (xf86Info.mseType + MICROSOFT == MOUSESYS)
        xf86Info.mouseFlags |= MF_CLEAR_DTR;
      else
        configError("ClearDTR only supported for MouseSystems mouse");
#else
      configError("ClearDTR not supported on this OS");
#endif
      break;
    case CLEARRTS:
#ifdef CLEARDTR_SUPPORT
      if (xf86Info.mseType + MICROSOFT == MOUSESYS)
        xf86Info.mouseFlags |= MF_CLEAR_RTS;
      else
        configError("ClearRTS only supported for MouseSystems mouse");
#else
      configError("ClearRTS not supported on this OS");
#endif
      break;
    default:
      pushToken = token;
      return;
    }
  }
}

/*
 * configGraphics --
 *      Set up all parameters for the graphics drivers. These may be changed
 *      by the driver during device-probe ...
 */

static void
configGraphics(scr_index)
     int scr_index;
{
  int token, i;
  DisplayModePtr pNew, pLast;
  Bool dummy = scr_index < 0 || !xf86Screens[scr_index];
  ScrnInfoPtr screen = NULL;

  if (dummy)
    screen = (ScrnInfoPtr)xalloc(sizeof(ScrnInfoRec));
  else
  {
    screen = xf86Screens[scr_index];
    screen->configured = TRUE;
    screen->tmpIndex = screenno++;
    screen->scrnIndex = scr_index;	/* scrnIndex must not be changed */
    screen->frameX0 = -1;
    screen->frameY0 = -1;
    screen->virtualX = -1;
    screen->virtualY = -1;
    screen->defaultVisual = -1;
    screen->chipset = NULL;
    screen->modes = NULL;
    OFLG_ZERO(&(screen->options));
    OFLG_ZERO(&(screen->xconfigFlag));
    screen->videoRam = 0;
    screen->width = 240;
    screen->height = 180;
    screen->speedup = SPEEDUP_DEFAULT;
    screen->clockprog = NULL;
    screen->bankedMono = FALSE;
    screen->textclock = -1;
    screen->blackColour.red = 0;
    screen->blackColour.green = 0;
    screen->blackColour.blue = 0;
    screen->whiteColour.red = 0x3F;
    screen->whiteColour.green = 0x3F;
    screen->whiteColour.blue = 0x3F;
  }
  screen->clocks = 0;
  textClockValue = -1;
  for (;;) {

    token = getToken(GraphicsTab);
    if (!dummy && !validateGraphicsToken(screen->validTokens, token))
    {
      char mesg[80];

      sprintf(mesg, "\"%s\" is not valid keyword for %s",
              tokenToString(GraphicsTab, token), screen->name);
      configError(mesg);
    }

    switch (token) {
    case STATICGRAY:
    case GRAYSCALE:
    case STATICCOLOR:
    case PSEUDOCOLOR:
    case TRUECOLOR:
    case DIRECTCOLOR:
      if (!dummy && screen->defaultVisual >= 0)
        configError("Only one default visual may be specified");
      screen->defaultVisual = token - STATICGRAY;
      if (!dummy && xf86Verbose) {
        char *visualname;
        switch (token) {
        case STATICGRAY:
          visualname = "StaticGray";
          break;
        case GRAYSCALE:
          visualname = "GrayScale";
          break;
        case STATICCOLOR:
          visualname = "StaticColor";
          break;
        case PSEUDOCOLOR:
          visualname = "PseudoColor";
          break;
        case TRUECOLOR:
          visualname = "TrueColor";
          break;
        case DIRECTCOLOR:
          visualname = "DirectColor";
          break;
        }
        ErrorF("%s %s: Default visual: %s\n", XCONFIG_GIVEN, screen->name,
               visualname);
      }
      
      break;

    case CHIPSET:
      if (getToken(NULL) != STRING) configError("Chipset string expected");
      screen->chipset = val.str;
      OFLG_SET(XCONFIG_CHIPSET,&(screen->xconfigFlag));
      break;

    case CLOCKS:
      OFLG_SET(XCONFIG_CLOCKS,&(screen->xconfigFlag));
      if ((token = getToken(NULL)) == STRING)
      {
         /* XXXX since we're using a bitmap for this, we should allow
	    multiple Clock strings (don't know if this will ever be used) */
#if 0
	 if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions)))
	 {
	    configError("Clock string already specified");
	    break;
	 }
#endif
	 if (screen->clocks == 0)
	 {
	    i = 0;
	    while (xf86_ClockOptionTab[i].token != -1)
	    {
	       if (StrCaseCmp(val.str, xf86_ClockOptionTab[i].name) == 0)
	       {
  		  OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions));
		  OFLG_SET(xf86_ClockOptionTab[i].token,
			   &(screen->clockOptions));

		  break;
	       }
	       i++;
	    }
	    if (xf86_ClockOptionTab[i].token == -1) {
	       configError("Unknown clock string");
	       break;
	    }
	 }
	 else
	 {
	    configError("Clocks previously specified by value");
	 }
	 break;
      }
      if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions)))
      {
	 configError("Clock previously specified as programmable");
	 break;
      }
      for (i = screen->clocks; token == NUMBER && i < MAXCLOCKS; i++) {
	screen->clock[i] = (int)(val.realnum * 1000.0 + 0.5);
	token = getToken(NULL);
      }

      screen->clocks = i;
      pushToken = token;
      break;

    case DISPLAYSIZE:
      OFLG_SET(XCONFIG_DISPLAYSIZE,&(screen->xconfigFlag));
      if (getToken(NULL) != NUMBER) configError("Display Width expected");
      screen->width = val.num;
      if (getToken(NULL) != NUMBER) configError("Display Height expected");
      screen->height = val.num;
      break;

    case MODES:
      for (pLast=NULL; (token = getToken(NULL)) == STRING; pLast = pNew)
	{
	  pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
	  pNew->name = val.str;

	  if (pLast) 
	    {
	      pLast->next = pNew;
	      pNew->prev  = pLast;
	    }
	  else
	    screen->modes = pNew;
	}
      pNew->next = screen->modes;
      screen->modes->prev = pLast;
      pushToken = token;
      break;

    case SCREENNO:
      if (getToken(NULL) != NUMBER) configError("Screen number expected");
      screen->tmpIndex = val.num;
      break;

    case OPTION:
      if (getToken(NULL) != STRING) configError("Option string expected");
      i = 0;
      while (xf86_OptionTab[i].token != -1) 
      {
	if (StrCaseCmp(val.str, xf86_OptionTab[i].name) == 0)
	{
          OFLG_SET(xf86_OptionTab[i].token, &(screen->options));
	  break;
	}
	i++;
      }
      if (xf86_OptionTab[i].token == -1)
        configError("Unknown option string");
      break;

    case VIDEORAM:
      OFLG_SET(XCONFIG_VIDEORAM,&(screen->xconfigFlag));
      if (getToken(NULL) != NUMBER) configError("Video RAM size expected");
      screen->videoRam = val.num;
      break;

    case VIEWPORT:
      OFLG_SET(XCONFIG_VIEWPORT,&(screen->xconfigFlag));
      if (getToken(NULL) != NUMBER) configError("Viewport X expected");
      screen->frameX0 = val.num;
      if (getToken(NULL) != NUMBER) configError("Viewport Y expected");
      screen->frameY0 = val.num;
      break;

    case VIRTUAL:
      OFLG_SET(XCONFIG_VIRTUAL,&(screen->xconfigFlag));
      if (getToken(NULL) != NUMBER) configError("Virtual X expected");
      screen->virtualX = val.num;
      if (getToken(NULL) != NUMBER) configError("Virtual Y expected");
      screen->virtualY = val.num;
      break;

    case SPEEDUP:
      OFLG_SET(XCONFIG_SPEEDUP,&(screen->xconfigFlag));
      if ((token = getToken(NULL)) == STRING)
	if (!strcmp(val.str,"all"))
	  screen->speedup = SPEEDUP_ALL;
	else
	  if (!strcmp(val.str,"best"))
	    screen->speedup = SPEEDUP_BEST;
	  else
	    if (!strcmp(val.str,"none"))
	      screen->speedup = 0;
            else
	      configError("Unrecognised SpeedUp option");
      else
      {
        pushToken = token;
	if ((token = getToken(NULL)) == NUMBER)
	  screen->speedup = val.num;
	else
	{
	  pushToken = token;
	  screen->speedup = SPEEDUP_ALL;
	}
      }
      break;
    case NOSPEEDUP:
      OFLG_SET(XCONFIG_SPEEDUP,&(screen->xconfigFlag));
      screen->speedup = 0;
      break;

    case CLOCKPROG:
      if (getToken(NULL) != STRING) configError("ClockProg string expected");
      if (val.str[0] != '/')
        FatalError("Full pathname must be given for ClockProg \"%s\"\n",
                   val.str);
      if (access(val.str, X_OK) < 0)
      {
        if (access(val.str, F_OK) < 0)
          FatalError("ClockProg \"%s\" does not exist\n", val.str);
        else
          FatalError("ClockProg \"%s\" is not executable\n", val.str);
      }
      {
        struct stat stat_buf;
        stat(val.str, &stat_buf);
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
          FatalError("ClockProg \"%s\" is not a regular file\n", val.str);
      }
      screen->clockprog = val.str;
      if (!dummy && xf86Verbose)
        ErrorF("%s %s: ClockProg: \"%s\"", XCONFIG_GIVEN, screen->name,
               screen->clockprog);
      if (getToken(NULL) == NUMBER)
      {
        textClockValue = (int)(val.realnum * 1000.0 + 0.5);
	if (!dummy && xf86Verbose)
          ErrorF(", Text Clock: %7.3f\n", textClockValue / 1000.0);
      }
      else
      {
        pushToken = token;
	if (!dummy && xf86Verbose)
          ErrorF("\n");
      }
      break;

    case BIOSBASE:
      if (getToken(NULL) != NUMBER) configError("BIOS base address expected");
      screen->BIOSbase = val.num;
      if (!dummy && xf86Verbose)
        ErrorF("%s %s: BIOS Base Address: %x\n", XCONFIG_GIVEN, screen->name,
	       val.num);
      break;

    case MEMBASE:
      if (getToken(NULL) != NUMBER) configError("Memory base address expected");
      screen->MemBase = val.num;
      if (!dummy && xf86Verbose)
        ErrorF("%s %s: Memory Base Address: %x\n", XCONFIG_GIVEN, screen->name,
	       val.num);
      break;

    case BLACK:
    case WHITE:
      {
        unsigned char rgb[3];
        int i, savetoken;
        
        savetoken = token;
        for (i = 0; i < 3; i++)
        {
          if (getToken(NULL) != NUMBER) configError("RGB value expected");
          rgb[i] = val.num & 0x3F;
        }
        if (savetoken == BLACK)
        {
          screen->blackColour.red = rgb[0];
          screen->blackColour.green = rgb[1];
          screen->blackColour.blue = rgb[2];
        }
        else
        {
          screen->whiteColour.red = rgb[0];
          screen->whiteColour.green = rgb[1];
          screen->whiteColour.blue = rgb[2];
        }
      }
      break;

    default:
      pushToken = token;
      if (dummy && screen != NULL)
      {
        xfree(screen);
        screen = NULL;
      }
      return;
    }
  }
}

void
xf86DeleteMode(infoptr, dispmp)
ScrnInfoPtr	infoptr;
DisplayModePtr	dispmp;
{
	if(infoptr->modes == dispmp)
		infoptr->modes = dispmp->next;

	if(dispmp->next == dispmp)
		FatalError("No valid modes found.\n");

	ErrorF("Removing mode %s from list of valid modes.\n",dispmp->name);
	dispmp->prev->next = dispmp->next;
	dispmp->next->prev = dispmp->prev;

	xfree(dispmp->name);
	xfree(dispmp);
}

/*
 * findConfigFile --
 * 	Locate the Xconfig file.  Abort if not found.
 */
static void
findConfigFile(filename, fp)
      char *filename;
      FILE **fp;
{
  char           *home;
  char           *xconfig;
  char	         *xwinhome = NULL;

#define configFile (*fp)
#define configPath filename

  /*
   * First open if necessary the config file.
   * If the -xconfig flag was used, use the name supplied there.
   * If $XCONFIG is a pathname, use it as the name of the config file
   * If $XCONFIG is set but doesn't contain a '/', append it to 'Xconfig'
   *   and search the standard places.
   * If $XCONFIG is not set, just search the standard places.
   */
  while (!configFile) {
    
    /*
     * First check if the -xconfig option was used.
     */
    if (xf86ConfigFile[0]) {
      strcpy(configPath, xf86ConfigFile);
      if (configFile = fopen(configPath, "r"))
        break;
      else
        FatalError(
             "Cannot read file \"%s\" specified by the -xconfig flag\n",
             configPath);
    }
    /*
     * Check if XCONFIG is set.
     */
    if (xconfig = getenv("XCONFIG")) {
      if (index(xconfig, '/')) {
        strcpy(configPath, xconfig);
        if (configFile = fopen(configPath, "r"))
          break;
        else
          FatalError(
               "Cannot read file \"%s\" specified by XCONFIG variable\n",
               configPath);
      }
    }
     
    /*
     * ~/Xconfig ...
     */
    if (home = getenv("HOME")) {
      strcpy(configPath,home);
      strcat(configPath,"/Xconfig");
      if (xconfig) strcat(configPath,xconfig);
      if (configFile = fopen( configPath, "r" )) break;
    }
    
    /*
     * /etc/Xconfig
     */
    strcpy(configPath, "/etc/Xconfig");
    if (xconfig) strcat(configPath,xconfig);
    if (configFile = fopen( configPath, "r" )) break;
    
    /*
     * $(LIBDIR)/Xconfig.<hostname>
     */

    if ((xwinhome = getenv("XWINHOME")) != NULL)
	sprintf(configPath, "%s/lib/X11/Xconfig", xwinhome);
    else
	strcpy(configPath, SERVER_CONFIG_FILE);
    if (xconfig) strcat(configPath,xconfig);
    strcat(configPath, ".");
#ifdef AMOEBA
    {
      extern char *XServerHostName;

      strcat(configPath, XServerHostName);
    }
#else
    gethostname(configPath+strlen(configPath), MAXHOSTNAMELEN);
#endif
    if (configFile = fopen( configPath, "r" )) break;
    
    /*
     * $(LIBDIR)/Xconfig
     */
    if (xwinhome)
	sprintf(configPath, "%s/lib/X11/Xconfig", xwinhome);
    else
	strcpy(configPath, SERVER_CONFIG_FILE);
    if (xconfig) strcat(configPath,xconfig);
    if (configFile = fopen( configPath, "r" )) break;
    
    FatalError("No config file found!\n");
  }
  if (xf86Verbose) {
    ErrorF("Xconfig: %s\n", configPath);
    ErrorF("%s stands for supplied, %s stands for probed/default values\n",
       XCONFIG_GIVEN, XCONFIG_PROBED);
  }
#undef configFile
#undef configPath
}

/*
 * xf86Config --
 *	Fill some internal structure with userdefined setups. Many internal
 *      Structs are initialized.  The drivers are selected and initialized.
 *	if (! vtopen), Xconfig is read, but devices are not probed.
 *	if (vtopen), devices are probed (and modes resolved).
 *	The vtopen argument was added so that Xconfig information could be
 *	made available before the VT is opened.
 */
void
xf86Config (vtopen)
     int vtopen;
{
  int            token, scr_index;
  int            i, j;
  Bool           graphFound = FALSE;
#if defined(SYSV) || defined(linux) || defined(_MINIX)
  int            xcpipe[2];
#endif
  static DisplayModePtr pNew, pLast;

 if (!vtopen)
 {

  OFLG_ZERO(&GenericXconfigFlag);
  configBuf  = (char*)xalloc(CONFIG_BUF_LEN);
  configRBuf = (char*)xalloc(CONFIG_BUF_LEN);
  configPath = (char*)xalloc(PATH_MAX);
  
  configBuf[0] = '\0';                    /* sanity ... */
  
  /*
   * Read the Xconfig file with the real uid to avoid security problems
   *
   * For SYSV we fork, and send the data back to the parent through a pipe
   */

#if defined(SYSV) || defined(linux) || defined(_MINIX)
  if (getuid() != 0) {
    if (pipe(xcpipe))
      FatalError("Pipe failed (%s)\n", strerror(errno));
    switch (fork()) {
      case -1:
        FatalError("Fork failed (%s)\n", strerror(errno));
        break;
      case 0:   /* child */
        close(xcpipe[0]);
        setuid(getuid());  
        findConfigFile(configPath, &configFile);
        {
          unsigned char pbuf[CONFIG_BUF_LEN];
          int nbytes;

          /* Pass the filename back as the first line */
          strcat(configPath, "\n");
          if (write(xcpipe[1], configPath, strlen(configPath)) < 0)
            FatalError("Child error writing to pipe (%s)\n", strerror(errno));
          while ((nbytes = fread(pbuf, 1, CONFIG_BUF_LEN, configFile)) > 0)
            if (write(xcpipe[1], pbuf, nbytes) < 0)
              FatalError("Child error writing to pipe (%s)\n", strerror(errno));
        }
        close(xcpipe[1]);
        fclose(configFile);
        exit(0);
        break;
      default: /* parent */
        close(xcpipe[1]);
        configFile = (FILE *)fdopen(xcpipe[0], "r");
        if (fgets(configPath, PATH_MAX, configFile) == NULL)
          FatalError("Error reading config file\n");
        configPath[strlen(configPath) - 1] = '\0';
    }
  }
  else {
    findConfigFile(configPath, &configFile);
  }
#else /* ! (SYSV || linux) */
  {
    int real_uid = getuid();

#if !defined(SVR4) && !defined(__NetBSD__) && !defined(__FreeBSD__)
    setruid(0);
#endif
    seteuid(real_uid);
    findConfigFile(configPath, &configFile);
    seteuid(0);
#if !defined(SVR4) && !defined(__NetBSD__) && !defined(__FreeBSD__)
    setruid(real_uid);
#endif
  }
#endif /* SYSV || linux */
  xf86Info.sharedMonitor = FALSE;
  xf86Info.kbdProc = NULL;
  xf86Info.mseProc = NULL;
  xf86Info.notrapSignals = FALSE;
  xf86Info.caughtSignal = FALSE;

  
  while ((token = getToken(SymTab)) != EOF)
    
    switch (token) {
      
    case SHAREDMON:
      xf86Info.sharedMonitor = TRUE;
      break;
      
    case FONTPATH:
      OFLG_SET(XCONFIG_FONTPATH,&GenericXconfigFlag);
      if (getToken(NULL) != STRING) configError("Font path component expected");
      j = FALSE;
      if (fontPath == NULL)
	{
	  fontPath = (char *)xalloc(1);
	  fontPath[0] = '\0';
	  i = strlen(val.str) + 1;
	}
      else
	{
          i = strlen(fontPath) + strlen(val.str) + 1;
          if (fontPath[strlen(fontPath)-1] != ',') 
	    {
	      i++;
	      j = TRUE;
	    }
	}
      fontPath = (char *)xrealloc(fontPath,i);
      if (j)
        strcat(fontPath, ",");
      strcat(fontPath, val.str);
      xfree(val.str);
      break;
      
    case RGBPATH:
      OFLG_SET(XCONFIG_RGBPATH,&GenericXconfigFlag);
      if (getToken(NULL) != STRING) configError("RGB path expected");
      if (!xf86coFlag)
        rgbPath = val.str;
      break;
      
    case NOTRAPSIGNALS:
      xf86Info.notrapSignals=TRUE;
      break;
      
    case KEYBOARD:
      xf86Info.kbdProc    = xf86KbdProc;
#ifdef AMOEBA
      xf86Info.kbdEvents  = NULL;
#else
      xf86Info.kbdEvents  = xf86KbdEvents;
#endif
      configKeyboard();
      break;
      
      
    case MICROSOFT:
    case MOUSESYS:
    case MMSERIES:
    case LOGITECH:
    case BUSMOUSE:
    case LOGIMAN:	/* see xf86Io.c for details [CHRIS-211092] */
    case PS_2:
    case MMHITTAB:
      if (getToken(NULL) != STRING) configError("Mouse device expected");
#ifdef AMOEBA
      xf86Info.mseProc    = xf86MseProc;
      xf86Info.mseEvents  = NULL;
#else
      xf86Info.mseProc    = xf86MseProc;
      xf86Info.mseEvents  = xf86MseEvents;
#endif
      xf86Info.mseType    = token - MICROSOFT;
#ifdef MACH386
      xf86Info.mseDevice  = "/dev/mouse";
#else
      xf86Info.mseDevice  = val.str;
#endif
      if (!xf86MouseSupported(xf86Info.mseType))
      {
        configError("Mouse type not supported by this OS");
      }
      configMouse();
      if (xf86Verbose)
      {
        char *mouseType = "unknown";
        Bool formatFlag = FALSE;
        switch (token)
        {
        case MICROSOFT:
          mouseType = "Microsoft";
          break;
        case MOUSESYS:
          mouseType = "MouseSystems";
          break;
        case MMSERIES:
          mouseType = "MMSeries";
          break;
        case LOGITECH:
          mouseType = "Logitech";
          break;
        case BUSMOUSE:
          mouseType = "BusMouse";
          break;
        case LOGIMAN:
          mouseType = "MouseMan";
          break;
        case PS_2:
          mouseType = "PS/2";
          break;
        case MMHITTAB:
          mouseType = "HitachiTablet";
          break;
        }
        ErrorF("%s Mouse: type: %s, device: %s", 
           XCONFIG_GIVEN, mouseType, xf86Info.mseDevice);
        if (token != BUSMOUSE && token != PS_2)
        {
          formatFlag = TRUE;
          ErrorF(", baudrate: %d", xf86Info.baudRate);
        }
        if (xf86Info.sampleRate)
        {
          ErrorF("%ssamplerate: %d", formatFlag ? ",\n       " : ", ",
                 xf86Info.sampleRate);
          formatFlag = !formatFlag;
        }
        if (xf86Info.emulate3Buttons)
          ErrorF("%s3 button emulation", formatFlag ? ",\n       " : ", ");
        if (xf86Info.chordMiddle)
          ErrorF("%sChorded middle button", formatFlag ? ",\n       " : ", ");
        ErrorF("\n");
      }
      
      break;
      
#ifdef XQUEUE
    case XQUE:
      xf86Info.kbdProc   = xf86XqueKbdProc;
      xf86Info.kbdEvents = xf86XqueEvents;
      xf86Info.mseProc   = xf86XqueMseProc;
      xf86Info.mseEvents = xf86XqueEvents;
      xf86Info.xqueSema  = 0;
      configKeyboard();
      configMouse();
      if (xf86Verbose)
        ErrorF("%s Xqueue selected for mouse and keyboard input\n",
	       XCONFIG_GIVEN);
      break;
#endif
      
#ifdef USE_OSMOUSE
    case OSMOUSE:
      if (xf86Verbose)
        ErrorF("%s OsMouse selected for mouse input\n", XCONFIG_GIVEN);
      /*
       *  allow an option to be passed to the OsMouse routines
       */
      if ((i = getToken(NULL)) != ERROR_TOKEN)
	xf86OsMouseOption(i, (pointer) &val);
      else
	pushToken = i;
      xf86Info.mseProc   = xf86OsMouseProc;
      xf86Info.mseEvents = xf86OsMouseEvents;
      configMouse();
	  break;
#endif
      
    case VGA256:
    case VGA2:
    case MONO:
    case VGA16:
    case ACCEL:
      scr_index = getScreenIndex(token);
      configGraphics(scr_index);
      /* Check for information that must be specified in Xconfig */
      if (scr_index >= 0 && xf86Screens[scr_index])
      {
        ScrnInfoPtr driver = xf86Screens[scr_index];

        graphFound = TRUE;

        if (driver->clockprog && !driver->clocks)
        {
          FatalError(
            "%s: When ClockProg is specified a Clocks line is required\n",
            driver->name);
        }
        if (validateGraphicsToken(driver->validTokens, MODES) && !driver->modes)
        {
          FatalError("%s: A Modes line must be specified in Xconfig\n",
                     driver->name);
        }
        /* Find the Index of the Text Clock for the ClockProg */
        if (driver->clockprog && textClockValue > 0)
        {
          Bool found_clock = FALSE;

          for (j = 0; j < NUM_PASSES; j++)
          {
            for (i=0; i < driver->clocks; i++)
	      if (abs(textClockValue - driver->clock[i]) <= tol_table[j])
              {
	        found_clock = TRUE;
                driver->textclock = i;
                break;
              }
            if (found_clock)
              break;
          }
          if (found_clock && xf86Verbose)
            ErrorF("$s %s: text clock = %7.3f, clock used = %7.3f\n",
              XCONFIG_GIVEN,
              driver->name, textClockValue / 1000.0,
              driver->clock[driver->textclock] / 1000.0);
          if (!found_clock)
            FatalError(
             "There is no defined dot-clock matching the text clock\n");
        }
        if (defaultColorVisualClass < 0)
          defaultColorVisualClass = driver->defaultVisual;
      }
      break;

    case MODEDB:
      for (pLast=NULL, token = getToken(NULL);
	   token == STRING || token == NUMBER;
	   pLast = pNew)
	{
	  pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
	  if (pLast) 
	    pLast->next = pNew;
	  else
	    pModes = pNew;
	  
	  if (token == STRING)
	    {
	      pNew->name = val.str;
	      if ((token = getToken(NULL)) != NUMBER)
		FatalError("Dotclock expected");
	    }
	  else if (pLast)
	    {
#if defined(MACH) || defined(AMOEBA) || defined(_MINIX)
              pNew->name = (char *) xalloc (strlen (pLast->name) + 1);
              strcpy (pNew->name, pLast->name);
#else
	      pNew->name = (char *)strdup(pLast->name);
#endif
	    }
	  else
	    FatalError("Mode name expected");

          pNew->next = NULL;
          pNew->prev = NULL;
	  pNew->Flags = 0;
	  pNew->Clock = (int)(val.realnum * 1000.0 + 0.5);
	  
	  if (getToken(NULL) == NUMBER) pNew->HDisplay = val.num;
	  else configError("Horizontal display expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->HSyncStart = val.num;
	  else configError("Horizontal sync start expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->HSyncEnd = val.num;
	  else configError("Horizontal sync end expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->HTotal = val.num;
	  else configError("Horizontal total expected");
	  
	  
	  if (getToken(NULL) == NUMBER) pNew->VDisplay = val.num;
	  else configError("Vertical display expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->VSyncStart = val.num;
	  else configError("Vertical sync start expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->VSyncEnd = val.num;
	  else configError("Vertical sync end expected");
	  
	  if (getToken(NULL) == NUMBER) pNew->VTotal = val.num;
	  else configError("Vertical total expected");

	  while (((token=getToken(TimingTab)) != EOF) &&
		 (token != STRING) &&
		 (token != NUMBER))
	    
	    switch(token) {
	      
	    case INTERLACE: pNew->Flags |= V_INTERLACE;  break;
	    case PHSYNC:    pNew->Flags |= V_PHSYNC;     break;
	    case NHSYNC:    pNew->Flags |= V_NHSYNC;     break;
	    case PVSYNC:    pNew->Flags |= V_PVSYNC;     break;
	    case NVSYNC:    pNew->Flags |= V_NVSYNC;     break;
	    case CSYNC:     pNew->Flags |= V_CSYNC;      break;
	    default:
	      configError("Videomode special flag expected");
	      break;
	    }
	}
      pushToken = token;
      break;

    default:
      configError("Keyword expected");
      break;
    }
  
  fclose(configFile);
  Xfree(configBuf);
  Xfree(configRBuf);
  Xfree(configPath);

#if defined(SYSV) || defined(linux)
  if (getuid() != 0) {
    /* Wait for the child */
    wait(NULL);
  }
#endif
  
  /* Try Xconfig FontPath first */
  if (!xf86fpFlag)
    if (fontPath) {
      char *f = xf86ValidateFontPath(fontPath);
      if (*f)
        defaultFontPath = f;
      else
        ErrorF(
        "Warning: FontPath is completely invalid.  Using compiled-in default.\n"
              );
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
    else
      ErrorF("Warning: No FontPath specified, using compiled-in default.\n");
  else    /* Use fontpath specified with '-fp' */
  {
    OFLG_CLR (XCONFIG_FONTPATH, &GenericXconfigFlag);
    if (fontPath)
    {
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
  }
  if (!fontPath) {
    /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
    char *f = (char *)xalloc(strlen(defaultFontPath) + 1);
    f[0] = '\0';
    strcpy (f, defaultFontPath);
    defaultFontPath = xf86ValidateFontPath(f);
    xfree(f);
  }
  else
    xfree(fontPath);

  /* If defaultFontPath is still empty, exit here */

  if (! *defaultFontPath)
    FatalError("No valid FontPath could be found\n");
  if (xf86Verbose)
    ErrorF("%s FontPath set to \"%s\"\n", 
      OFLG_ISSET(XCONFIG_FONTPATH, &GenericXconfigFlag) ? XCONFIG_GIVEN :
      XCONFIG_PROBED, defaultFontPath);

  if (!xf86Info.kbdProc)
    FatalError("You must specify a keyboard in Xconfig");
  if (!xf86Info.mseProc)
    FatalError("You must specify a mouse in Xconfig");

  if (!graphFound)
  {
    Bool needcomma = FALSE;

    ErrorF("\nYou must provide a section in Xconfig for at least one of the\n");
    ErrorF("following graphics drivers: ");
    for (i = 0; i < xf86MaxScreens; i++)
    {
      if (!xf86Screens[i])
      {
        ErrorF("%s%s", needcomma ? ", " : "",
               tokenToString(SymTab, xf86ScreenNames[i]));
        needcomma = TRUE;
      }
    }
    ErrorF("\n");
    FatalError("No configured graphics devices");
  }
 }
 else	/* if (vtopen) */
 {
  /*
   * Probe all configured screens for letting them resolve their modes
   */
  for ( i=0; i < xf86MaxScreens; i++ )
    if (xf86Screens[i] && xf86Screens[i]->configured &&
	(xf86Screens[i]->configured = (xf86Screens[i]->Probe)()))
      xf86InitViewport(xf86Screens[i]);

  /*
   * Now sort the drivers to match the order of the ScreenNumbers
   * requested by the user. (sorry, slow bubble-sort here)
   * Note, that after this sorting the first driver that is not configured
   * can be used as last-mark for all configured ones.
   */
  for ( j = 0; j < xf86MaxScreens-1; j++)
    for ( i=0; i < xf86MaxScreens-j-1; i++ )
      if (!xf86Screens[i] || !xf86Screens[i]->configured ||
	  (xf86Screens[i+1] && xf86Screens[i+1]->configured &&
	   (xf86Screens[i+1]->tmpIndex < xf86Screens[i]->tmpIndex)))
	{
	  ScrnInfoPtr temp = xf86Screens[i+1];
	  xf86Screens[i+1] = xf86Screens[i];
	  xf86Screens[i] = temp;
	}

  /*
   * free up mode info...
   */
  if (pModes)
    for (pLast = pModes, pNew = pModes->next; pLast;)
    {
      Xfree(pLast->name);
      Xfree(pLast);
      pLast = pNew;
      if (pNew) pNew = pNew->next;
    }
 }
}


Bool 
xf86LookupMode(target, driver)
     DisplayModePtr target;
     ScrnInfoPtr    driver;
{
  DisplayModePtr p;
  int            i, j, mode_clock;
  Bool           found_mode = FALSE;
  Bool           found_clock = FALSE;
  Bool           clock_too_high = FALSE;
  static Bool	 first_time = TRUE;

  if (first_time)
  {
    ErrorF("%s %s: Maximum allowed dot-clock: %dMHz\n", XCONFIG_PROBED,
	   driver->name, driver->maxClock / 1000);
    first_time = FALSE;
  }

  for (j = 0; j < NUM_PASSES; j++)
  {
    for (p = pModes; p != NULL; p = p->next)    /* scan list */
    {
      if (!strcmp(p->name, target->name))       /* names equal ? */
      {
        if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(driver->clockOptions))) {
	    if (driver->clocks == 0) {
	    /* this we know */
	       driver->clock[0] = 25175;	/* 25.175Mhz */
	       driver->clock[1] = 28322;	/* 28.322MHz */
	       driver->clocks = 2;
	    }

	    if ((p->Clock / 1000) > (driver->maxClock / 1000))
	       clock_too_high = TRUE;
	    else
	    {
	       /* We fill in the the programmable clocks as we go */
               for (i=0; i < driver->clocks; i++)
                  if (driver->clock[i] ==  p->Clock)
                     break;

               if (i >= MAXCLOCKS)
	       {
                  ErrorF("Too many programable clocks used (limit %d)!\n",
			     MAXCLOCKS);
		return FALSE;
	       }

	       if (i == driver->clocks)
	       {
                   driver->clock[i] = p->Clock;
	           driver->clocks++;
	       }
	       
               found_clock = TRUE;
	       mode_clock = p->Clock;
	       target->Clock      = i;
	       target->HDisplay   = p->HDisplay;
	       target->HSyncStart = p->HSyncStart;
	       target->HSyncEnd   = p->HSyncEnd;
	       target->HTotal     = p->HTotal;
	       target->VDisplay   = p->VDisplay;
	       target->VSyncStart = p->VSyncStart;
	       target->VSyncEnd   = p->VSyncEnd;
	       target->VTotal     = p->VTotal;
	       target->Flags      = p->Flags;
	    }
	}
	else
	 for (i=0; i < driver->clocks; i++)      /* scan clocks */
	  if (abs(p->Clock - driver->clock[i]) <= tol_table[j])
	  {
	    if ((driver->clock[i] / 1000) > (driver->maxClock / 1000))
	    {
	      clock_too_high = TRUE;
	      break;
	    }
	    found_clock = TRUE;
            mode_clock = p->Clock;
	    target->Clock      = i;
	    target->HDisplay   = p->HDisplay;
	    target->HSyncStart = p->HSyncStart;
	    target->HSyncEnd   = p->HSyncEnd;
	    target->HTotal     = p->HTotal;
	    target->VDisplay   = p->VDisplay;
	    target->VSyncStart = p->VSyncStart;
	    target->VSyncEnd   = p->VSyncEnd;
	    target->VTotal     = p->VTotal;
	    target->Flags      = p->Flags;
            break;
	  }
        found_mode = TRUE;
      }
    }
    if (found_clock && xf86Verbose)
      if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(driver->clockOptions))) 
	 ErrorF("%s %s: Mode \"%s\": mode clock = %7.3f\n",
              XCONFIG_GIVEN,driver->name, target->name, mode_clock / 1000.0);
      else
	 ErrorF("%s %s: Mode \"%s\": mode clock = %7.3f, clock used = %7.3f\n",
              XCONFIG_GIVEN,driver->name, target->name, mode_clock / 1000.0,
              driver->clock[target->Clock] / 1000.0);       
    if (found_clock || !found_mode) break;
  }
  if (!found_mode)
    {
    ErrorF("There is no mode definition named \"%s\"\n", target->name);
    return FALSE;
    }
  if (!found_clock && clock_too_high)
    {
    ErrorF("Clocks for mode \"%s\" %s\n\tLimit is %7.3f (MHz)\n",
	       target->name,
	       "are too high for the configured hardware.",
	       driver->maxClock / 1000.0);
    return FALSE;
    }
  if (!found_clock)
    {
    ErrorF("There is no defined dot-clock matching mode \"%s\"\n",
               target->name);
    return FALSE;
    }
  return TRUE;
}

void
xf86VerifyOptions(allowedOptions, driver)
     OFlagSet      *allowedOptions;
     ScrnInfoPtr    driver;
{
  int j;

  for (j=0; xf86_OptionTab[j].token >= 0; j++)
    if ((OFLG_ISSET(xf86_OptionTab[j].token, &driver->options)))
      if (OFLG_ISSET(xf86_OptionTab[j].token, allowedOptions))
      {
	if (xf86Verbose)
	  ErrorF("%s %s: Option \"%s\"\n", XCONFIG_GIVEN,
	         driver->name, xf86_OptionTab[j].name);
      }
      else
	ErrorF("%s %s: Option flag \"%s\" is not defined for this driver\n",
	       XCONFIG_GIVEN, driver->name, xf86_OptionTab[j].name);
}
