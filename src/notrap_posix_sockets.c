#include <notrap/notrap.h>
#ifdef NTP_POSIX_THREADS

struct NTPSock_struct {
	int sock;
	char destination[5000];
	BOOL error;

	//Indicates the connect thread is running
	volatile BOOL doingConnect;
	
	//indicates NTPDisconnect() was called while the connect
	//thread was running
	volatile BOOL shouldInterruptConnect;

	//This lock protects the 'doingConnect' variable. No access
	//should be done to that variable outside of that lock.
	//It also protects 'shouldInterruptConnect.' No writes should
	//be done to that variable outside of that lock.
	NTPLock *connectLock;

};


//------------------------------------------------------------------
// Functions for connecting and disconnecting
//------------------------------------------------------------------

//There's no reasonable way to do DNS lookups in a non-blocking way,
//so we're going to simulate it with a single thread
static void doLookupAndConnectInSeparateThread(void *obj) {
	NTPSock *sock = (NTPSock*)obj;

	
	NTPAcquireLock(sock->connectLock);
	sock->doingConnect = NO;
	NTPReleaseLock(sock->connectLock);

	//Check to see if our connect got interrupted.
	//If it did, we need to cleanup ourselves.
	if(sock->shouldInterruptConnect==YES) {
		NTPDisconnect(&sock);
	}
}


NTPSock *NTPConnectTCP(const char *destination) {
	NTPSock *rv = (NTPSock*)malloc(sizeof(NTPSock));
	if(rv==NULL) goto ERR_NO_MEM;

	rv->connectLock = NTPNewLock();
	if(rv->connectLock==NULL) goto ERR_CONNECT_LOCK;

	//initialize our struct
	rv->sock = -1;
	rv->error = FALSE;
	rv->shouldInterruptConnect = YES;
	rv->doingConnect = YES;
	strncpy(rv->destination, destination, sizeof(rv->destination)-1);
	rv->destination[sizeof(rv->destination)-1] = 0;

	//begin the asynchronous connect
	if(!NTPStartThread(doLookupAndConnectInSeparateThread,rv))
		goto ERR_START_THREAD;
	
	return rv; //success


ERR_START_THREAD:
	NTPFreeLock(&rv->connectLock);

ERR_CONNECT_LOCK:
	free(rv);
	rv=NULL;

ERR_NO_MEM:
	return rv;
}

void NTPDisconnect(NTPSock **sock) {
	NTPLock *lock = (*sock)->connectLock;
	if(sock==NULL || *sock==NULL) return;

	//Complications always come when you're using threads,
	//and here's ours. If we're connecting while the thread
	//is running, we need to signal to the connect method
	//to clean things up. We need to do it with a lock
	//because otherwise we have a race condition. If we weren't
	//using threads, we wouldn't need to do all this locking
	NTPAcquireLock(lock);
	if((*sock)->doingConnect==YES) {
		//we are still connecting, so tell our thread to free it
		(*sock)->shouldInterrupConnect = YES;
		NTPReleaseLock(lock);
	} else{
		//we are no longer connecting, so we can free everything ourselves
		NTPReleaseLock(lock);

		//Here is where we actually free everything
		NTPFreeLock(&(*sock)->lock);
		if((*sock)->sock >=0) close((*sock)->sock);
		free(*sock);
	}
	//In either case, set *sock to NULL so the end user
	//can't use it anymore.
	*sock = NULL;
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

