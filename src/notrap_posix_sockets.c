#include <notrap/notrap.h>
#ifdef NTP_POSIX_THREADS

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

const char *CONNECTING_ERR_MSG = "Waiting for connect.....";

struct NTPSock_struct {
	int sock;
	char destination[5000];
	int  port;

	//errMsg holds the most recent error in a human
	//readable format
	char errMsg[2000];

	//Indicates the connect thread is running.
	//No other thread than the connec thread 
	//has a right to modify the NTPSock_struct 
	//while this is true.
	volatile BOOL doingConnect;


	//True if there was an error while connecting
	BOOL connectError;
	
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
	struct addrinfo hints, *servinfo, *p;
	char port[50];

	//Please Lord, may I never have to write one of these again.
	//Here is where we actually do the lookup.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	sprintf(port, "%d",sock->port);
	if((rv = getaddrinfo(sock->destination, port, &hints, &servinfo))!=0){
		//deal with error
		snprintf(sock->errMsg, sizeof(sock->errMsg), "DNS lookup, %s",
		         gai_strerror(rv));
		sock->errMsg[sizeof(sock->errMsg)-1]=0;
		goto ERR;
	}
	
	//got the lookup, now get ourselves a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if((sock->sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0){
			snprintf(sock->errMsg, sizeof(sock->errMsg), "sock() failed, %s",
			           strerror(errno));
			sock->errMsg[sizeof(sock->errMsg)-1]=0;
			//we can try again until we run out of p->ai_next
			continue;
		}

		//and if we got the socket, try to connect
		if(connect(sockfd, p->ai_addr, p->ai_addrlen) <0) {
			snprintf(sock->errMsg, sizeof(sock->errMsg), 
			        "connect to %s failed, %s\n", sock->destination,strerror(errno));
			sock->errMsg[sizeof(sock->errMsg)-1]=0;
			close(sock->sock);
			//we can try again until we run out of p->ai_next
			continue;
		}
		
		break; //connection successful! p has our value,
		       //but if we don't have success, p will be NULL
	}

	//don't dereference 'p' after this
	freeaddrinfo(servinfo);

	if(p==NULL) {
		//connection unsuccessful on all attempts!
		//we already stored the error message of the last error
		goto ERR;
	}

	//success! We are connected, but we still need to
	//mark our connect as done
	goto SIGNAL_CONNECTION_COMPLETE;

ERR:
	//Only run this line if there was an error. But the rest
	//always needs to be run. If you have trouble with GOTOs, sorry.
	//The alternative is still uglier.
	sock->connectError = TRUE;	


SIGNAL_CONNECTION_COMPLETE:
	//Check to see if our connect got interrupted by a disconnect
	//If it did, we need to cleanup ourselves.
	NTPAcquireLock(sock->connectLock); {

		//The 'sock' is guaranteed to not be freed until
		//this is set to NO. And it can only be set while
		//we hold this lock.
		sock->doingConnect = NO;
		if(sock->shouldInterruptConnect==YES) {
			NTPDisconnect(&sock);
			NTPReleaseLock(sock->connectLock);
		}
	
	} NTPReleaseLock(sock->connectLock);
}

/**Allocates memory for an NTPSock and fills it in with some
 * good defaults.*/
static NTPSock *allockNTPSock(const char *destination, uint16_t port) {
	NTPSock *rv = (NTPSock*)malloc(sizeof(NTPSock));
	if(rv!=NULL) {
		rv->sock = -1;
		rv->port = port;
		rv->connectError = FALSE;
		rv->shouldInterruptConnect = NO;
		rv->doingConnect = YES;
		strncpy(rv->destination, destination, sizeof(rv->destination)-1);
		rv->destination[sizeof(rv->destination)-1] = 0;
		strcpy(rv->errMsg, "No error, yet");
	}
	return rv;
}


NTPSock *NTPConnectTCP(const char *destination, uint16_t port) {
	NTPSock *rv = allocNTPSock(destination);
	if(rv==NULL) goto ERR_NO_MEM;

	rv->connectLock = NTPNewLock();
	if(rv->connectLock==NULL) goto ERR_CONNECT_LOCK;


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


NTPSock*NTPListen(uint16_t port) {
	NTPSock *rv = allocNTPSock("", port);
	if(rv==NULL) return NULL;
	
}

NTPSock*NTPAccept(NTPSock *listenPort) {

}

//------------------------------------------------------------------
// Status methods
//------------------------------------------------------------------
int NTPSockStatus(NTPSock *sock) {

}

const char*NTPSockErr(NTPSock*sock) {
	if(sock->doingConnect) 
		return CONNECTING_ERR_MSG;
	
	else
		return sock->errMsg;

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

