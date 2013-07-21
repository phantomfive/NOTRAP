/******************************************************************
 * notrap_posix_sockets.c                                         *
 * Prepare yourself, networking code is always ugly. But the      *
 * organization should be clear from the header file.             *
 * -AT Copyright 2013 Usable under the GPL 3.0 or later           *
 ******************************************************************/

#include <notrap/notrap.h>
#ifdef NTP_POSIX_THREADS

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static const char *CONNECTING_ERR_MSG = "Waiting for connect.....";

//------------------------------------------------------------------
// Our data structures
//------------------------------------------------------------------

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

	//True if this is a server socket, used for listening
	BOOL listenSock; 

	//True if there was an error while connecting
	BOOL connectError;

	//True if there was an error while trying to listen on a port
	BOOL listenError;
	
	//indicates NTPDisconnect() was called while the connect
	//thread was running
	volatile BOOL shouldInterruptConnect;

	//This lock protects the 'doingConnect' variable. No access
	//should be done to that variable outside of that lock.
	//It also protects 'shouldInterruptConnect.' No writes should
	//be done to that variable outside of that lock.
	NTPLock *connectLock;

};


//-----------------------------------------------------------------
// Function for initializing general networking. Only gets run
// once the first time a socket is created.
// Mainly we use it to handle SIGPIPE
//-----------------------------------------------------------------
static BOOL networkInitialized = FALSE;
static void initNetwork() {
	if(networkInitialized) return;
	
	signal(SIGPIPE, SIG_IGN);
	networkInitialized = TRUE;
}

//------------------------------------------------------------------
// Functions for doing DNS Lookup. This is insane
//------------------------------------------------------------------

//There's no reasonable way to do DNS lookups in a non-blocking way,
//so we're going to simulate it with a single thread
static void *doLookupAndConnectInSeparateThread(void *obj) {
	NTPSock *sock = (NTPSock*)obj;
	struct addrinfo hints, *servinfo, *p;
	int rv;
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
		if(connect(sock->sock, p->ai_addr, p->ai_addrlen) <0) {
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
	NTPAcquireLock(sock->connectLock);  {

		//The 'sock' is guaranteed to not be freed until
		//this is set to NO. And it can only be set while
		//we hold this lock.
		sock->doingConnect = NO;
		if(sock->shouldInterruptConnect==YES) {
			NTPReleaseLock(sock->connectLock);
			NTPDisconnect(&sock);
		}
		else{
			NTPReleaseLock(sock->connectLock);
		}
	}

	return NULL;
}

//------------------------------------------------------------------
// Functions for connecting and disconnecting
//------------------------------------------------------------------

/**Allocates memory for an NTPSock and fills it in with some
 * good defaults.*/
static NTPSock *allocNTPSock(const char *destination, uint16_t port) {
	initNetwork();

	NTPSock *rv = (NTPSock*)malloc(sizeof(NTPSock));
	if(rv!=NULL) {
		rv->shouldInterruptConnect = FALSE;
		rv->sock         = -1   ;
		rv->port         = port ;
		rv->connectError = FALSE;
		rv->listenError  = FALSE;
		rv->listenSock   = FALSE;
		rv->doingConnect = FALSE;
		strncpy(rv->destination, destination, sizeof(rv->destination)-1);
		rv->destination[sizeof(rv->destination)-1] = 0;
		strcpy(rv->errMsg, "No error, yet");
	
		rv->connectLock = NTPNewLock();
		if(rv->connectLock==NULL) {
			free(rv);
			rv = NULL;
		}
	}

	return rv;
}


NTPSock *NTPConnectTCP(const char *destination, uint16_t port) {
	NTPSock *rv = allocNTPSock(destination, port);
	if(rv==NULL) goto ERR_NO_MEM;



	//begin the asynchronous connect
	rv->doingConnect = TRUE;
	if(!NTPStartThread(doLookupAndConnectInSeparateThread,rv))
		goto ERR_START_THREAD;
	
	return rv; //success


ERR_START_THREAD:
	NTPFreeLock(&rv->connectLock);
	free(rv);
	rv=NULL;

ERR_NO_MEM:
	return rv;
}

void NTPDisconnect(NTPSock **sock) {
	if(sock==NULL || *sock==NULL) return;
	NTPLock *lock = (*sock)->connectLock;

	//Complications always come when you're using threads,
	//and here's ours. If we're connecting while the thread
	//is running, we need to signal to the connect method
	//to clean things up. We need to do it with a lock
	//because otherwise we have a race condition. If we weren't
	//using threads, we wouldn't need to do all this locking
	NTPAcquireLock(lock);
	if((*sock)->doingConnect==YES) {
		//we are still connecting, so tell our thread to free it
		(*sock)->shouldInterruptConnect = YES;
		NTPReleaseLock(lock);
	} else{
		//we are no longer connecting, so we can free everything ourselves
		NTPReleaseLock(lock);
		
		//Here is where we actually free everything
		NTPFreeLock(&(*sock)->connectLock);
		if((*sock)->sock >=0) close((*sock)->sock);
		free(*sock);
	}

	//In either case, set *sock to NULL so the end user
	//can't use it anymore.
	*sock = NULL;
}


NTPSock*NTPListen(uint16_t port) {
	struct addrinfo hints, *servinfo, *p;
	int ev;
	char portStr[20];

	NTPSock *rv = allocNTPSock("", port);
	if(rv==NULL) return NULL;
	rv->listenSock = TRUE;
	
	sprintf(portStr, "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if((ev=getaddrinfo(NULL, portStr, &hints, &servinfo))!=0) {
		rv->listenError = TRUE;
		snprintf(rv->errMsg, sizeof(rv->errMsg), "GetAddrInfo Err, %s",
		         gai_strerror(ev));
		return rv;
	}

	rv->sock = -1;
	for(p=servinfo; p!=NULL; p = p->ai_next) {
		int optval = 1;
		if((rv->sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0) {
			snprintf(rv->errMsg, sizeof(rv->errMsg), "Socket not created, %s",
			         strerror(errno));
			continue;
		}

		//if this one fails, it's alright, can keep going
		setsockopt(rv->sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

		if(bind(rv->sock, p->ai_addr, p->ai_addrlen) <0) {
			snprintf(rv->errMsg, sizeof(rv->errMsg), "Couldn't bind, %s",
			         strerror(errno));
			close(rv->sock);
			rv->sock = -1;
			continue;
		}

		break;
	}
	freeaddrinfo(servinfo);
	
	if(rv->sock>=0) {

		//this one needs to work, though
		if(listen(rv->sock, 100)<0) {
			snprintf(rv->errMsg, sizeof(rv->errMsg), "couldn't listen, %s",
		   	      strerror(errno));
			close(rv->sock);
			rv->sock = -1;
		}

	}

	if(rv->sock==-1) {
		rv->listenError = TRUE;
	}
	return rv;
}


NTPSock *NTPAccept(NTPSock *sock) {
	int acceptedSock;
	struct sockaddr address;
	NTPSock *rv;
	socklen_t address_len;

	if(!sock->listenSock) {
		snprintf(sock->errMsg, sizeof(sock->errMsg), "Socket is not listening");
		return NULL;
	}
	
	acceptedSock = accept(sock->sock, &address, &address_len);
	if(acceptedSock<0) {
		snprintf(sock->errMsg,sizeof(sock->errMsg),"accepting, %s",strerror(errno));
		return NULL;
	}
	
	rv = allocNTPSock("", -1);
	if(rv==NULL) {
		snprintf(sock->errMsg, sizeof(sock->errMsg), "no memory");
		close(acceptedSock);
		return NULL;
	}

	rv->sock = acceptedSock;
	return rv;
}

//------------------------------------------------------------------
// Status methods
//------------------------------------------------------------------
int NTPSockStatus(NTPSock *sock) {

	//test for errors first
	if(sock->connectError || sock->listenError) {
		return NTPSOCK_ERROR;
	}

	//If we get here, we're not in an error state (yet)
	else if(sock->doingConnect) {
		return NTPSOCK_CONNECTING;
	}
	else if(sock->listenSock) {
		return NTPSOCK_LISTENING;
	}
	else {
		return NTPSOCK_CONNECTED;
	}
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
	int rv;

	if(sock->doingConnect) return -1;

	if((rv=send(sock->sock, bytes, len, 0))<0) {
		snprintf(sock->errMsg, sizeof(sock->errMsg),"sending, %s",strerror(errno));
		return -1;
	}

	return rv;
}

int NTPRecv(NTPSock *sock, void *buf, int len) {
	int rv;
	if(sock->doingConnect) return -1;

	if((rv=recv(sock->sock, buf, len, 0))<0) {
		snprintf(sock->errMsg,sizeof(sock->errMsg),"recving, %s",strerror(errno));
		return -1;
	}

	return rv;
}

//------------------------------------------------------------------
// Methods for select
//------------------------------------------------------------------
struct NTP_FD_SET_struct {
	fd_set set;
	uint16_t max;
};

void NTP_ZERO_SET(NTP_FD_SET *set) {
	memset(set, 0, sizeof(struct NTP_FD_SET_struct));
}

void NTP_FD_ADD(NTPSock *sock, NTP_FD_SET *set) {
	FD_SET(sock->sock, &set->set);
	if(sock->sock>set->max) 
		set->max = sock->sock;
}

BOOL NTP_FD_ISSET(NTPSock *sock, NTP_FD_SET *set) {
	return (BOOL)FD_ISSET(sock->sock, &set->set);
}

int NTPSelect(NTP_FD_SET *readSet, NTP_FD_SET *writeSet, int timeoutMS) {
	struct timeval tv;
	int max = (readSet->max>writeSet->max) ? readSet->max : writeSet->max;

	tv.tv_sec  = timeoutMS / 1000;
	tv.tv_usec = (timeoutMS%1000)*1000;

	return select(max + 1, &readSet->set, &writeSet->set, NULL, &tv);
}


#endif

