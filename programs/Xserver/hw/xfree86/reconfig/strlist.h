/* $XFree86$ */

/* Used in the %union, therefore to be included in the scanner. */
typedef struct {
	int count;
	char **datap;
} string_list ;

typedef struct {
	int count;
	string_list **datap;
} string_list_list;
