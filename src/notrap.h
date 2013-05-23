#ifndef NOTRAP_H
#define NOTRAP_H

/*************************************************************************
 * This is the primary header file for NOTRAP. It should be all
 * that the user needs to include. Here are defined all the functions 
 * available, though for some platforms we use macros since there
 * are not huge differences.
 *
 * It's best to keep this as small as possible, because then porting to
 * new platforms is easier.
 *
 * We use #ifdefs to make sure code only gets compiled for the various
 * platforms.
 * For code that is defined for OSX, we use #ifdef NTP_OSX
 * For code that is defined for Windows, we use #ifdef NTP_WOE
 * For code that is defined for Linux, we use #ifdef NTP_LIN
 *
 * Copyright 2013 Andrew Thompson. Usable under the GPL3.0 or later.
 ***********************************************************************/

#ifdef __APPLE__
#define NTP_OSX 1
#include "notrap_osx.h"
#endif


/***********************************************************************
 * Section for constants, like BOOL
 ***********************************************************************/
#ifndef BOOL
#define BOOL char
#endif
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/**********************************************************************
 * Section for various standard functions.
 * Believe it or not, some platforms don't even suppor the standard
 * library exactly.
 **********************************************************************/
//various includes
#ifdef NTP_OSX
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define NTPmalloc malloc
#define NTPstrlen strlen
#define NTPmemcpy memcpy
#define NTPstrcpy strcpy
#endif

/**********************************************************************
 * Section for Networking functions.
 * Networking is tough, because there are more differences here than
 * with most other types of functions.
 *********************************************************************/











#endif

