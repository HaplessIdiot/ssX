/* $XFree86$ */

/* 
 * Main procedure for XF86Setup, by Joe Moss
 */

#define USE_PHASE3 0

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <tcl.h>
#include <tk.h>
#include <errno.h>

static Tk_Window mainWindow;
static Tcl_Interp *interp;

static char *name = NULL, *LibDir;

#define PHASE1	"/phase1.tcl"
#define PHASE2	"/phase2.tcl"
#define PHASE3	"/phase3.tcl"

static char Set_InitVars[] =
	"if [info exists env(XWINHOME)] {\n"
	"    set Xwinhome $env(XWINHOME)\n"
	"} else {\n"
	"    set xdirs [list /usr/X11R6 /usr/X11 "
		"/usr/X /var/X11R6 /var/X11 /var/X /usr/X11R6.1 "
		"/usr/local/X11R6 /usr/local/X11 /usr/local/X]\n"
	"    foreach dir $xdirs {\n"
	"        if {[llength [glob -nocomplain $dir/bin/XF86_*]] } {\n"
	"            set Xwinhome $dir\n"
	"            break\n"
	"        }\n"
	"    }\n"
	"    if ![info exists Xwinhome] {\n"
	"        error \"Couldn't determine where you have XFree86 installed.\\n"
		"If you have XFree86 properly installed, set the "
		"XWINHOME environment\\n variable to point to the "
		"parent directory of the XFree86 bin directory.\\n\"\n"
	"    }\n"
	"    unset xdirs dir\n"
	"}\n"
	"if [info exists env(XF86SETUPLIB)] {\n"
	"    set XF86Setup_library $env(XF86SETUPLIB)\n"
	"} else {\n"
	"    set XF86Setup_library $Xwinhome/lib/X11/XF86Setup\n"
	"}\n"
	"set tk_library [set tcl_library $XF86Setup_library/tcllib]\n"
	"set XF86Setup_startup $XF86Setup_library" PHASE1 "\n"
	"if ![file exists $XF86Setup_startup] {\n"
	"    error \"The startup file for this program ($XF86Setup_startup)\\n"
		"is missing. You need to install it before running "
		"this program.\\n\"\n"
	"} else {\n"
	"    if ![file readable $XF86Setup_startup] {\n"
	"        error \"The startup file for this program "
		"($XF86Setup_startup)\\ncan't be accessed. "
		"Perhaps a permission problem?\\n\"\n"
	"    }\n"
	"}\n";

static int	XF86Setup_TclEvalFile(
#if NeedNestedProtoTypes
	Tcl_Interp *interp,
	char *filename
#endif
);

static int
XF86Setup_TclEvalFile(interp, filename)
    Tcl_Interp *interp;
    char *filename;
{
    int retval;
    char *msg;

    retval = Tcl_VarEval(interp, "source ", LibDir, filename, (char *) NULL);
    if (retval != TCL_OK) {
	msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
	if (msg == NULL) {
	    msg = interp->result;
	}
	fprintf(stderr, "%s\n", msg);
	Tcl_Eval(interp, "exit 1");
    }
    fflush(stdout);
    Tcl_ResetResult(interp);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    char *args, *Xwinhome, *argv0, *class, tmpbuf[20];

    /****  Create the Tcl interpreter  ****/
    interp = Tcl_CreateInterp();

    /****  Add the commands to the Tcl interpreter for the
           convenience functions ****/

    if (XF86Other_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Add the commands to the Tcl interpreter that interface
           with the Cards database ****/

    if (Cards_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Add the commands to the Tcl interpreter that interface
           with the XF86Config reading routines ****/

    if (XF86Config_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Find where things are installed ****/
    if (Tcl_Eval(interp, Set_InitVars) != TCL_OK) {
	fprintf(stderr, interp->result);
	exit (1);
    }

    LibDir = Tcl_GetVar(interp, "XF86Setup_library", TCL_GLOBAL_ONLY);

    /**** This program will not be used interactively as a shell ****/
    Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

    /**** Parse command-line arguments.  ****/

    argv0 = argv[0];
    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    XtFree(args);
    sprintf(tmpbuf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", tmpbuf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", argv0, TCL_GLOBAL_ONLY);

    /****  Execute the Phase I Tcl code  ****/
    XF86Setup_TclEvalFile(interp, PHASE1);

    /**** Set the name used as the class (basename(argv[0])) ****/

    name = strrchr(argv0, '/');
    if (name != NULL) {
        name++;
    } else {
        name = argv0;
    }

    class = (char *) XtMalloc((unsigned) (strlen(name) + 1));
    strcpy(class, name);
    class[0] = toupper((unsigned char) class[0]);

    /**** Here is the first routine that needs to have an X
	  server running.  It tries to create a window and will,
	  of course, fail if the server isn't running ****/

    mainWindow = Tk_CreateMainWindow(interp, NULL, name, class);
    if (mainWindow == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }

    /****  Add the commands to the Tcl interpreter that interface
           with the XFree86-VidModeExtension ****/

    if (XF86vid_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Add the commands to the Tcl interpreter that interface
           with the XFree86-Misc extension ****/

    if (XF86Misc_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Add the commands to the Tcl interpreter that interface
           with the XKEYBOARD extension and library ****/

    if (XF86Kbd_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /****  Now execute the Phase II commands ****/

    XF86Setup_TclEvalFile(interp, PHASE2);

    /**** Enter the event loop until phase2 is completed (last
	window destroyed) ****/

    Tk_MainLoop();

#if USE_PHASE3 /* It is possible to open a connection to the new server
	        using the -screen option to the toplevel command.  We'll
	        try doing it that way instead of using a third phase with
	        it's own MainWindow and event loop */

    /**** Phase II should have set the DISPLAY environment variable
	  appropriately for the new server.  Now we need to open
	  a connection and main window on that server ****/

    mainWindow = Tk_CreateMainWindow(interp, NULL, name, class);
    if (mainWindow == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }

    /****  Lastly, execute the Phase III commands ****/

    XF86Setup_TclEvalFile(interp, PHASE3);

    /**** Enter the event loop until phase3 is completed ****/

    Tk_MainLoop();
#endif

    XtFree(class);
    Tcl_Eval(interp, "exit 0");
    exit(1);

}

void keypress() {
/****  The parse_database routine (in cards.c) calls keypress() when it
 * finds a problem with the database (after printing an error message)
 * What do we want to do with it? 
 * This is what xf86config does:
	printf("Press enter to continue, or ctrl-c to abort.");
	getchar();
	printf("\n");
****/
}

