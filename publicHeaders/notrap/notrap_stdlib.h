#ifndef NOTRAP_STDLIB_H
#define NOTRAP_STDLIB_H

/**********************************************************************
 * Section for various standard functions.
 * Believe it or not, some platforms don't even suppor the standard
 * library exactly.
 * Copyright 2013 Andrew Usable under the GPL 3.0 or greater
 **********************************************************************/
//various includes
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NTPmalloc malloc
#define NTPfree   free
#define NTPstrlen strlen
#define NTPmemcpy memcpy
#define NTPstrcpy strcpy
#define NTPstrcmp strcmp



#endif

