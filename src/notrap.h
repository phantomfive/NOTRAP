#ifndef NOTRAP_H
#define NOTRAP_H

/*************************************************************************
 * This is the primary header file for NOTRAP.
 * It defines all the functions available, and for some platforms,
 * defines macros because we don't need a full function call.
 *
 * We use #ifdefs to make sure code only gets compiled for the various
 * platforms.
 * For code that is defined for OSX, we use #ifdef NOP_OSX
 * For code that is defined for Windows, we use #ifdef NOP_WOE
 * For code that is defined for Linux, we use #ifdef NOP_LIN
 *
 * Copyright 2013 Andrew Thompson. Usable under the GPL3.0 or later.
 ***********************************************************************/

#ifdef __APPLE__
#define NOP_OSX 1
#endif


/**********************************************************************
 * Section for Networking functions
 *********************************************************************/
//various includes
#ifdef NOP_OSX

#endif











#endif

