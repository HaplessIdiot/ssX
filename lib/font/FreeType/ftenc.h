/* $XFree86$ */

/* This structure represents a cmap and eventually a recoding
 * function.  In order to convert a code into an index, we first apply
 * the recoding function and then the cmap.
 * pid=-1 terminates the list.
 * pid=-2 means any Unicode table.
 * pid=-3 means any Apple Unicode table. */

struct ttf_encoding_alternative {
  int pid, eid;
  unsigned (*recode)(unsigned, void*);
  void *client_data;
  struct ttf_encoding_alternative *next;
};

/* This is the structure that holds the info for one charset.  It
 * consists of a charset name, its size, and an alternative like
 * above.  For efficiency reasons, there should be no recoding
 * functions for large charsets (but it should still work). */

struct ttf_encoding_info {
  char *charset;
  int size;
  struct ttf_encoding_alternative *alternatives;
  struct ttf_encoding_info *next;
};

/* This is a private structure that represents a `simple' remapping. */

struct ttf_simple_remapping {
  unsigned short first;
  unsigned short len;
  unsigned short *map;
};

unsigned ttf_simple_remap(unsigned, void*);
struct ttf_encoding_info* loadEncodingFile(char*, char*);
