/* $XFree86: xc/programs/Xserver/hw/xfree86/config/scan.c,v 1.1 1998/01/24 16:57:46 hohndel Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#if !defined(X_NOT_POSIX)
#if defined(_POSIX_SOURCE)
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif /* _POSIX_SOURCE */
#endif /* !X_NOT_POSIX */
#if !defined(PATH_MAX)
#if defined(MAXPATHLEN)
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif /* MAXPATHLEN */
#endif /* !PATH_MAX */

#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN 32
#endif /* !MAXHOSTNAMELEN */

#include "X11/Xos.h"
#include "Configint.h"
#include "xf86tokens.h"

extern char xf86ConfigFile[];

#define CONFIG_BUF_LEN     1024

static int StringToToken (char *, xf86ConfigSymTabRec *);

static FILE *configFile = NULL;
static int configStart = 0;		/* start of the current token */
static int configPos = 0;		/* current readers position */
static int configLineNo = 0;	/* linenumber */
static char *configBuf, *configRBuf;	/* buffer for lines */
static char *configPath;		/* path to config file */
static char *configSection;		/* name of current section being parsed */
static int pushToken = LOCK_TOKEN;
LexRec val;

/* 
 * StrToUL --
 *
 *  A portable, but restricted, version of strtoul().  It only understands
 *  hex, octal, and decimal.  But it's good enough for our needs.
 */
unsigned int
StrToUL (char *str)
{
	int base = 10;
	char *p = str;
	unsigned int tot = 0;

	if (*p == '0')
	{
		p++;
		if (*p == 'x')
		{
			p++;
			base = 16;
		}
		else
			base = 8;
	}
	while (*p)
	{
		if ((*p >= '0') && (*p <= ((base == 8) ? '7' : '9')))
		{
			tot = tot * base + (*p - '0');
		}
		else if ((base == 16) && (*p >= 'a') && (*p <= 'f'))
		{
			tot = tot * base + 10 + (*p - 'a');
		}
		else if ((base == 16) && (*p >= 'A') && (*p <= 'F'))
		{
			tot = tot * base + 10 + (*p - 'A');
		}
		else
		{
			return (tot);
		}
		p++;
	}
	return (tot);
}

/* 
 * xf86GetToken --
 *      Read next Token form the config file. Handle the global variable
 *      pushToken.
 */
int
xf86GetToken (xf86ConfigSymTabRec * tab)
{
	int c, i;

	/* 
	 * First check whether pushToken has a different value than LOCK_TOKEN.
	 * In this case rBuf[] contains a valid STRING/TOKEN/NUMBER. But in the
	 * oth * case the next token must be read from the input.
	 */
	if (pushToken == EOF_TOKEN)
		return (EOF_TOKEN);
	else if (pushToken == LOCK_TOKEN)
	{

		c = configBuf[configPos];

		/* 
		 * Get start of next Token. EOF is handled,
		 * whitespaces & comments are* skipped. 
		 */
		do
		{
			if (!c)
			{
				if (fgets (configBuf, CONFIG_BUF_LEN - 1, configFile) == NULL)
				{
					return (pushToken = EOF_TOKEN);
				}
				configLineNo++;
				configStart = configPos = 0;
			}
#ifndef __EMX__
			while (((c = configBuf[configPos++]) == ' ') || (c == '\t') || (c == '\n'));
#else
			while (((c = configBuf[configPos++]) == ' ') || (c == '\t') || (c == '\n')
				   || (c == '\r'));
#endif
			if (c == '#')
				c = '\0';
		}
		while (!c);

		/* GJA -- handle '-' and ','  * Be careful: "-hsync" is a keyword. */
		if ((c == ',') && !isalpha (configBuf[configPos]))
		{
			configStart = configPos;
			return COMMA;
		}
		else if ((c == '-') && !isalpha (configBuf[configPos]))
		{
			configStart = configPos;
			return DASH;
		}

		configStart = configPos;
		/* 
		 * Numbers are returned immediately ...
		 */
		if (isdigit (c))
		{
			int base;

			if (c == '0')
				if ((configBuf[configPos] == 'x') ||
					(configBuf[configPos] == 'X'))
					base = 16;
				else
					base = 8;
			else
				base = 10;

			configRBuf[0] = c;
			i = 1;
			while (isdigit (c = configBuf[configPos++]) ||
				   (c == '.') || (c == 'x') ||
				   ((base == 16) && (((c >= 'a') && (c <= 'f')) ||
									 ((c >= 'A') && (c <= 'F')))))
				configRBuf[i++] = c;
			configPos--;		/* GJA -- one too far */
			configRBuf[i] = '\0';
			val.num = StrToUL (configRBuf);
			val.realnum = atof (configRBuf);
			return (NUMBER);
		}

		/* 
		 * All Strings START with a \" ...
		 */
		else if (c == '\"')
		{
			i = -1;
			do
			{
				configRBuf[++i] = (c = configBuf[configPos++]);
#ifndef __EMX__
			}
			while ((c != '\"') && (c != '\n') && (c != '\0'));
#else
			}
			while ((c != '\"') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
			configRBuf[i] = '\0';
			val.str = (char *) xf86confmalloc (strlen (configRBuf) + 1);
			strcpy (val.str, configRBuf);	/* private copy ! */
			return (STRING);
		}

		/* 
		 * ... and now we MUST have a valid token.  The search is
		 * handled later along with the pushed tokens.
		 */
		else
		{
			configRBuf[0] = c;
			i = 0;
			do
			{
				configRBuf[++i] = (c = configBuf[configPos++]);;
#ifndef __EMX__
			}
			while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\0'));
#else
			}
			while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
			configRBuf[i] = '\0';
			i = 0;
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

		if (temp == COMMA || temp == DASH)
			return (temp);
		if (temp == NUMBER || temp == STRING)
			return (temp);
	}

	/* 
	 * Joop, at last we have to lookup the token ...
	 */
	if (tab)
	{
		i = 0;
		while (tab[i].token != -1)
			if (StrCaseCmp (configRBuf, tab[i].name) == 0)
				return (tab[i].token);
			else
				i++;
	}

	return (ERROR_TOKEN);		/* Error catcher */
}

void
xf86UnGetToken (int token)
{
	pushToken = token;
}

char *
xf86TokenString (void)
{
	return configRBuf;
}

/* 
 * xf86OpenConfigFile --
 *
 * Formerly findConfigFile(). This function take a pointer to a location
 * in which to place the actual name of the file that was opened.
 * This function uses the global character array xf86ConfigFile
 * This function returns the following results.
 *
 *  0   unable to open the config file
 *  1   file opened and ready to read
 *  
 */

int
xf86OpenConfigFile (char *filename)
{
#define MAXPTRIES   6
	char *home = NULL;
	char *xconfig = NULL;
	char *xwinhome = NULL;
	char *configPaths[MAXPTRIES];
	int pcount = 0, idx;

/* 
 * First open if necessary the config file.
 * If the -xf86config flag was used, use the name supplied there (root only).
 * If $XF86CONFIG is a pathname, use it as the name of the config file (root)
 * If $XF86CONFIG is set but doesn't contain a '/', append it to 'XF86Config'
 *   and search the standard places (root only).
 * If $XF86CONFIG is not set, just search the standard places.
 */
	configFile = NULL;
	configStart = 0;		/* start of the current token */
	configPos = 0;		/* current readers position */
	configLineNo = 0;	/* linenumber */
	pushToken = LOCK_TOKEN;
	while (!configFile)
	{

		/* 
		 * configPaths[0]         is used as a buffer for -xf86config
		 *                  and $XF86CONFIG if it contains a path
		 * configPaths[1...MAXPTRIES-1] is used to store the paths of each of
		 *                  the other attempts
		 */
		for (pcount = idx = 0; idx < MAXPTRIES; idx++)
			configPaths[idx] = NULL;

		/* 
		 * First check if the -xf86config option was used.
		 */
		configPaths[pcount] = (char *) xf86confmalloc (PATH_MAX);
		if (xf86ConfigFile[0])
		{
			strcpy (configPaths[pcount], xf86ConfigFile);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
			else
				return 0;
		}
		/* 
		 * Check if XF86CONFIG is set.
		 */
#ifndef __EMX__
		if (getuid () == 0
			&& (xconfig = getenv ("XF86CONFIG")) != 0
			&& index (xconfig, '/'))
#else
		/* no root available, and filenames start with drive letter */
		if ((xconfig = getenv ("XF86CONFIG")) != 0
			&& isalpha (xconfig[0])
			&& xconfig[1] == ':')
#endif
		{
			strcpy (configPaths[pcount], xconfig);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
			else
				return 0;
		}

#ifndef __EMX__
		/* 
		 * ~/XF86Config ...
		 */
		if (getuid () == 0 && (home = getenv ("HOME")) != NULL)
		{
			configPaths[++pcount] = (char *) xf86confmalloc (PATH_MAX);
			strcpy (configPaths[pcount], home);
			strcat (configPaths[pcount], "/" XCONFIGFILE);
			if (xconfig)
				strcat (configPaths[pcount], xconfig);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
		}

		/* 
		 * /etc/XF86Config
		 */
		configPaths[++pcount] = (char *) xf86confmalloc (PATH_MAX);
		strcpy (configPaths[pcount], "/etc/" XCONFIGFILE);
		if (xconfig)
			strcat (configPaths[pcount], xconfig);
		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;

		/* 
		 * $(XCONFIGDIR)/XF86Config.<hostname>
		 */

		configPaths[++pcount] = (char *) xf86confmalloc (PATH_MAX);
		if (getuid () == 0 && (xwinhome = getenv ("XWINHOME")) != NULL)
			sprintf (configPaths[pcount], "%s/lib/X11/" XCONFIGFILE, xwinhome);
		else
			strcpy (configPaths[pcount], XCONFIGDIR "/" XCONFIGFILE);
		if (getuid () == 0 && xconfig)
			strcat (configPaths[pcount], xconfig);
		strcat (configPaths[pcount], ".");
#ifdef AMOEBA
		{
			extern char *XServerHostName;

			strcat (configPaths[pcount], XServerHostName);
		}
#else
		gethostname (configPaths[pcount] + strlen (configPaths[pcount]),
					 MAXHOSTNAMELEN);
#endif
		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;
#endif /* !__EMX__  */

		/* 
		 * $(XCONFIGDIR)/XF86Config
		 */
		configPaths[++pcount] = (char *) xf86confmalloc (PATH_MAX);
#ifndef __EMX__
		if (getuid () == 0 && xwinhome)
			sprintf (configPaths[pcount], "%s/lib/X11/" XCONFIGFILE, xwinhome);
		else
			strcpy (configPaths[pcount], XCONFIGDIR "/" XCONFIGFILE);
		if (getuid () == 0 && xconfig)
			strcat (configPaths[pcount], xconfig);
#else
		/*
		 * we explicitly forbid numerous config files everywhere for OS/2;
		 * users should consider them lucky to have one in a standard place
		 * and another one with the -xf86config option
		 */
		xwinhome = getenv ("X11ROOT");	/* get drive letter */
		if (!xwinhome)
			FatalError ("X11ROOT environment variable not set\n");
		strcpy (configPaths[pcount], __XOS2RedirRoot ("/XFree86/lib/X11/XConfig"));
#endif

		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;

		return 0;
	}
	configBuf = (char *) xf86confmalloc (CONFIG_BUF_LEN);
	configRBuf = (char *) xf86confmalloc (CONFIG_BUF_LEN);
	configPath = (char *) xf86confmalloc (PATH_MAX);

	strcpy (configPath, configPaths[pcount]);

	if (filename)
		strcpy (filename, configPaths[pcount]);
	for (idx = 0; idx <= pcount; idx++)
		if (configPaths[idx] != NULL)
			xf86conffree (configPaths[idx]);

	configBuf[0] = '\0';		/* sanity ... */

	return 1;
}

void
xf86CloseConfigFile (void)
{
	xf86conffree (configPath);
	xf86conffree (configRBuf);
	xf86conffree (configBuf);

	fclose (configFile);
}

void
xf86ParseError (char *format,...)
{
	va_list ap;

	fprintf (stderr, "Parse error on line %d of section %s in file %s\n",
			 configLineNo, configSection, configPath);
	fprintf (stderr, "\t");
	va_start (ap, format);
	vfprintf (stderr, format, ap);
	va_end (ap);

	fprintf (stderr, "\n");
}

void
xf86ValidationError (char *format,...)
{
	va_list ap;

	fprintf (stderr, "Data incomplete in file %s\n",
			 configPath);
	fprintf (stderr, "\t");
	va_start (ap, format);
	vfprintf (stderr, format, ap);
	va_end (ap);

	fprintf (stderr, "\n");
}

void
SetSection (char *section)
{
	configSection = section;
}

/* 
 * xf86GetToken --
 *  Lookup a string if it is actually a token in disguise.
 */
int
getStringToken (xf86ConfigSymTabRec * tab)
{
	return StringToToken (val.str, tab);
}

static int
StringToToken (char *str, xf86ConfigSymTabRec * tab)
{
	int i;

	for (i = 0; tab[i].token != -1; i++)
	{
		if (!StrCaseCmp (tab[i].name, str))
			return tab[i].token;
	}
	return (ERROR_TOKEN);
}
