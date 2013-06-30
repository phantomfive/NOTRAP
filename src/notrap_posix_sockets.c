#include <notrap/notrap.h>
#ifdef NTP_POSIX_THREADS

struct NTPSock_struct {
	int sock;
};


//------------------------------------------------------------------
// Functions for connecting and disconnecting
//------------------------------------------------------------------
NTPSock *NTPConnectTCP(const char *destination) {

}

void NTPDisconnect(NTPSock **sock) {

}


//------------------------------------------------------------------
// Status methods
//------------------------------------------------------------------
int NTPSockStatus(NTPSock *sock) {

}

const char*NTPSockErr(NTPSock*sock) {

}


//------------------------------------------------------------------
// Methods for sending and receiving
//------------------------------------------------------------------
int NTPSend(NTPSock *sock, void *bytes, int len) {

}

int NTPRecv(NTPSock *sock, void *buf, int len) {

}

//------------------------------------------------------------------
// Methods for select
//------------------------------------------------------------------
struct NTP_FD_SET_struct {

}
void NTP_FD_ADD(NTPSock *sock, NTP_FD_SET *set) {

}

BOOL NTP_FD_ISSET(NTPSock *sock, NTP_FD_SET *set) {

}

int NTPSelect(NTP_FD_SET *readSet, NTP_FD_SET *writeSet, int timeoutMS) {

}


#endif

