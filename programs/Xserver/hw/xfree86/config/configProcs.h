/* $XFree86: $ */
/* Device.c */
XF86ConfDevicePtr parseDeviceSection(void);
void printDeviceSection(FILE *cf, XF86ConfDevicePtr ptr);
void freeDeviceList(XF86ConfDevicePtr ptr);
int validateDevice(XF86ConfigPtr p);
XF86ConfDevicePtr xf86FindDevice(char *ident, XF86ConfDevicePtr p);
/* Files.c */
static char *prependRoot(char *pathname);
XF86ConfFilesPtr parseFilesSection(void);
void printFileSection(FILE *cf, XF86ConfFilesPtr ptr);
void freeFiles(XF86ConfFilesPtr p);
/* Flags.c */
XF86ConfFlagsPtr parseFlagsSection(void);
void printServerFlagsSection(FILE *f, XF86ConfFlagsPtr flags);
void freeFlags(XF86ConfFlagsPtr flags);
void xf86OptionListFree(XF86OptionPtr opt);
XF86OptionPtr xf86FindOption(XF86OptionPtr list, char *name);
char *xf86FindOptionValue(XF86OptionPtr list, char *name);
/* Keyboard.c */
XF86ConfKeyboardPtr parseKeyboardSection(void);
void printKeyboardSection(FILE *cf, XF86ConfKeyboardPtr ptr);
void freeKeyboard(XF86ConfKeyboardPtr ptr);
/* Layout.c */
XF86ConfLayoutPtr parseLayoutSection(void);
void printLayoutSection(FILE *cf, XF86ConfLayoutPtr ptr);
void freeLayoutList(XF86ConfLayoutPtr ptr);
void freeAdjacencyList(XF86ConfAdjacencyPtr ptr);
int validateLayout(XF86ConfigPtr p);
XF86ConfLayoutPtr xf86FindLayout(XF86ConfLayoutPtr list, char *name);
/* Module.c */
XF86LoadPtr parseModuleSubSection(XF86LoadPtr head, char *name);
XF86ConfModulePtr parseModuleSection(void);
void printModuleSection(FILE *cf, XF86ConfModulePtr ptr);
XF86LoadPtr addNewLoadDirective(XF86LoadPtr head, char *name, int type, XF86OptionPtr opts);
void freeModules(XF86ConfModulePtr ptr);
/* Monitor.c */
XF86ConfModeLinePtr parseModeLine(void);
XF86ConfModeLinePtr parseVerboseMode(void);
XF86ConfMonitorPtr parseMonitorSection(void);
void printMonitorSection(FILE *cf, XF86ConfMonitorPtr ptr);
void freeMonitorList(XF86ConfMonitorPtr ptr);
void freeModeLineList(XF86ConfModeLinePtr ptr);
XF86ConfMonitorPtr xf86FindMonitor(char *ident, XF86ConfMonitorPtr p);
XF86ConfModeLinePtr xf86FindModeLine(char *ident, XF86ConfModeLinePtr p);
/* Pointer.c */
XF86ConfPointerPtr parsePointerSection(void);
void printPointerSection(FILE *cf, XF86ConfPointerPtr ptr);
void freePointer(XF86ConfPointerPtr ptr);
/* Screen.c */
XF86ConfDisplayPtr parseDisplaySubSection(void);
XF86ConfScreenPtr parseScreenSection(void);
void printScreenSection(FILE *cf, XF86ConfScreenPtr ptr);
void freeScreenList(XF86ConfScreenPtr ptr);
void freeDisplayList(XF86ConfDisplayPtr ptr);
void freeModeList(XF86ModePtr ptr);
int validateScreen(XF86ConfigPtr p);
XF86ConfScreenPtr xf86FindScreen(char *ident, XF86ConfScreenPtr p);
XF86ConfDisplayPtr xf86FindDisplay(int depth, XF86ConfDisplayPtr p);
/* Vendor.c */
XF86ConfVendorPtr parseVendorSection(void);
void freeVendorList(XF86ConfVendorPtr p);
XF86ConfVendorPtr xf86FindVendor(XF86ConfVendorPtr list, char *name);
/* cpconfig.c */
int StrCaseCmp(const char *s1, const char *s2);
void main(void);
/* read.c */
XF86ConfigPtr xf86ReadConfigFile(void);
int validateConfig(XF86ConfigPtr p);
GenericListPtr addListItem(GenericListPtr head, GenericListPtr new);
void XF86FreeConfig(XF86ConfigPtr p);
/* scan.c */
unsigned int StrToUL(char *str);
int xf86GetToken(xf86ConfigSymTabRec *tab);
void xf86UnGetToken(int token);
char *xf86TokenString(void);
int xf86OpenConfigFile(char *filename);
void xf86CloseConfigFile(void);
void xf86ParseError(char *format, ...);
void xf86ValidationError(char *format, ...);
void SetSection(char *section);
int getStringToken(xf86ConfigSymTabRec *tab);
static int StringToToken(char *str, xf86ConfigSymTabRec *tab);
/* write.c */
int xf86WriteConfigFile(const char *filename, XF86ConfigPtr cptr);
