#ifndef NOTRAP_POSIX_SOCKETS_H
#define NOTRAP_POSIX_SOCKETS_H

/***************************************************************
 * Useful for defines that must be known by the compiler, but
 * don't need to be revealed to the end user, and just clutter
 * things up.
 *
 * Copyright Andrew 2013 Usable under the GPL 3.0 or greater
 ***************************************************************/


//only use this file if NTP_POSIX_SOCKETS is defined
#ifdef NTP_POSIX_SOCKETS

#include <stdint.h>
#include <sys/select.h>

struct NTP_FD_SET_struct {
	fd_set set;
	uint16_t max;
};
















#endif
#endif
