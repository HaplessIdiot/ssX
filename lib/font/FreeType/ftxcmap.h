/* Extension for iterating over Cmaps */

/* Find the first entry of a Cmap.
 * Returns -1 on failure; eventually also returns the id. */
int TT_CharMap_First(TT_CharMap, int*);

/* Find the next entry of Cmap.  Same return conventions. */
int TT_CharMap_Next(TT_CharMap, int, int*);


