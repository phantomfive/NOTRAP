#ifndef NOTRAP_H
#define NOTRAP_H

/*************************************************************************
 * This is the primary header file for NOTRAP. It should be all
 * that the user needs to include. Here are defined all the functions 
 * available, though for some platforms we use macros since there
 * are not huge differences.
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
 * Section for Networking functions.
 * Networking is tough, because there are more differences here than
 * with most other types of functions.
 *********************************************************************/
//various includes
#ifdef NOP_OSX

#endif











#endif

