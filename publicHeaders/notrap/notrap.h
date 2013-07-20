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
 *
 * Copyright 2013 Andrew Thompson. Usable under the GPL3.0 or later.
 ***********************************************************************/

//We use #ifdefs to make sure code only gets compiled for the various
//platforms.
//For code that is defined for OSX, we use #ifdef NTP_OSX
//For code that is defined for Windows, we use #ifdef NTP_WOE
//For code that is defined for Linux, we use #ifdef NTP_LIN
#ifdef __APPLE__
#define NTP_OSX
#define NTP_STDLIB_AVAILABLE
#define NTP_POSIX_SOCKETS
#define NTP_POSIX_THREADS
#include "notrap_osx.h"
#endif

//The standard library is not available on all platforms.
//But when it is, we can just use these functions directly.
//All these will be the same as the normal stdlib, but
//will be preceded by NTP. So...
//malloc() will be replaced by NTPmalloc(). strcpy by NTPstrcpy().
//
//Be aware that some functions, like snprintf(), return different
//things on different platforms.
#ifdef NTP_STDLIB_AVAILABLE
#include "notrap_stdlib.h"
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
#ifndef SUCCESS
#define SUCCESS 1
#endif
#ifndef FAIL
#define FAIL 0
#endif
#ifndef YES
#define YES 1
#endif
#ifndef NO
#define NO 0
#endif

/**********************************************************************
 * Sections for networking. These can't be POSIX because Windows
 * doesn't have true POSIX networking. So this section isn't as simple
 * as the standard library
 *********************************************************************/
typedef struct NTPSock_struct NTPSock;

/**Since it will be work to make this platform independent, we might as
 * well make some porcelain commands. 99% of the time I just want to
 * connect to a destination over TCP with a DNS lookup. That's what
 * this function does.
 * Returns a new sock immediately, before the connection is complete,
 * because DNS lookup can take minutes. To discover the status of the
 * connection, call NTPSockStatus()*/
NTPSock *NTPConnectTCP(const char *destination, uint16_t port);

//Returns the current status of the socket. This is especially
//useful during connection, and should be called frequently, because
//it's where the connect actually does its work.
//Returns one of the following:
#define NTPSOCK_CONNECTING   1 //If it is still connecting
#define NTPSOCK_CONNECTED    2 //If it has connected
#define NTPSOCK_LISTENING    3 //If the socket is listening
#define NTPSOCK_ERROR        4 //If there was an error. Call NTPSockErr()
                               //For a string representation of the error.
int NTPSockStatus(NTPSock *sock); 

/**Returns a human readable error message to represent the last error.*/
const char*NTPSockErr(NTPSock*sock);

/**Disconnects the sock and frees all associated resources.
 * Sets *sock to NULL.*/
void NTPDisconnect(NTPSock **sock);

/**Begins listening on a local port for incoming connections*/
NTPSock*NTPListen(uint16_t port);

/**Waits to accept a connection on a listening port.
 * Resources must be freed later by calling NTPDisconnect() */
NTPSock*NTPAccept(NTPSock *listenSock);

/**Sends bytes over the socket.
 * Returns the number of bytes written, or -1 if there is
 * an error. Not all bytes are guaranteed to be written.*/
int NTPSend(NTPSock *sock, void *bytes, int len);

/**Will try to read len bytes from sock, and store them
 * in buf. Returns the number of bytes actually read. */
int NTPRecv(NTPSock *sock, void *buf, int len);

/**Select is useful enough to include here, even if it is
 * the most confusing function ever written.*/
typedef struct NTP_FD_SET_struct NTP_FD_SET;
void NTP_FD_ADD(NTPSock *sock, NTP_FD_SET *set);
BOOL NTP_FD_ISSET(NTPSock *sock, NTP_FD_SET *set);
//clears a NTP_FD_SET()
void NTP_ZERO_SET(NTP_FD_SET *set);

/**The way to use this:
 * Create a write set. Add NTPSocks to the write set using NTP_FD_ADD()
 * Create a read set.  Add NTPSocks to the read  set using NTP_FD_ADD()
 * Call NTPSelect() with a timeout.
 *
 * As soon as one of sockets in readSet has data available for reading,
 * or one of the sockets in writeSet has space available for writing,
 * this function will return, and readSet and writeSet will be modified.
 *
 * To find out if a socket has data available for reading, or space 
 * available for writing, call NTP_FD_ISSET().
 *
 * Be sure to call NTP_ZERO_SET() to clear the sets before calling NTP_FD_ADD()
 *
 * RETURNS: a negative number on error (call NTPSockErr() to find out which
 * error), a positive number on success, or 0 on timeout. 
 * When a timeout occurs, it might not have taken up the entire timeoutMS
 * time period, so don't  rely on this as a timer.*/
int NTPSelect(NTP_FD_SET *readSet, NTP_FD_SET *writeSet, int timeoutMS);



/*************************************************************************
 * Section for threading. Another problematic section, because it
 * can be wildly different depending on the platform. Some platforms
 * don't even have threads, thus if you want to be truly platform
 * independent, avoid these altogether.
 *
 * In addition, we only provide bare-bones functionality because that's
 * all you really want if you're trying to avoid bugs.
 ************************************************************************/

typedef struct NTPLock_struct NTPLock;

/**Creates a new thread, returns TRUE on SUCCESS, FALSE on ERROR.
 * The new thread calls start_routing() upon starting, which is a
 * function you must define. Here is an example, starting a thread:
 *
 * void myRoutine(void *arg) {
 *    //do things in a new thread
 * }
 *
 * 
 * //earlier on, to create the new thread,
 *    if(NTPStartThread( &myRoutine, NULL )) {
 *       //success!
 *    }
 */
BOOL NTPStartThread(void *(*start_routine)(void *), void *arg);

/**Creates a new lock. Returns NULL on error*/
NTPLock *NTPNewLock();

/**Frees a lock. Sets *lock to NULL*/
void NTPFreeLock(NTPLock **lock);

/**Acquires the lock. Blocks until successful.*/
BOOL NTPAcquireLock(NTPLock *lock);

/**Releases a lock. */
void NTPReleaseLock(NTPLock *lock);




#endif

