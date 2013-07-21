#include <stdio.h>
#include <CuTest.h>
#include <unistd.h>
#include <notrap/notrap.h>

void testDisconnectWhileConnecting(CuTest *tc) {
	int i;

	//Since this is threaded on some platforms,
	//do it 10 times just in case.
	//Should run this in Valgrind to check for memory leaks
	for(i=0;i<10;i++) {
		NTPSock *sock = NTPConnectTCP("asdfasdfasdf.com",4444);
		CuAssertPtrNotNull(tc, sock);
		NTPDisconnect(&sock);
		CuAssert(tc, "Cleared sock", sock==NULL);
	}
}

void testConnect(CuTest *tc) {
	int port = 34593;
	int sent,bytesSent, recvd, bytesRecvd;
	char msg[] = "From this valley they say you are going";
	char msgRecv[1000] = {0};
	NTPSock *newSock;
	NTPSock *cSock;

	//Listen on sock
	NTPSock *lSock = NTPListen(port);
	CuAssertPtrNotNull(tc, lSock);
	if(NTPSockStatus(lSock)!=NTPSOCK_LISTENING)
		printf("Listen error: %s\n", NTPSockErr(lSock));
	CuAssert(tc, "Make sure listening", NTPSockStatus(lSock)==NTPSOCK_LISTENING);

	//connect to listening port
	cSock = NTPConnectTCP("localhost", port);
	CuAssertPtrNotNull(tc, cSock);
	
	//wait for connect to succeed or fail
	while(NTPSockStatus(cSock)==NTPSOCK_CONNECTING);
	if(NTPSockStatus(cSock)!=NTPSOCK_CONNECTED)
		printf("Connect error: %s\n", NTPSockErr(cSock));
	CuAssert(tc, "Testing connect", NTPSockStatus(cSock)==NTPSOCK_CONNECTED);

	
	//accept on listening port
	newSock = NTPAccept(lSock);
	CuAssertPtrNotNull(tc, newSock);

	//send a message
	for(sent=0;sent<strlen(msg);sent+=bytesSent) {
		bytesSent = NTPSend(cSock, msg, strlen(msg)-sent);
		CuAssert(tc, "Checking bytes sent didn't fail", bytesSent>0);
	}

	//recv the message
	for(recvd=0;recvd<strlen(msg);recvd+=bytesSent) {
		bytesRecvd = NTPRecv(newSock, msgRecv+recvd, strlen(msg)-recvd);
		CuAssert(tc, "Checking recv didn't fail", bytesRecvd>0);
	}

	//make sure the message is correct
	CuAssert(tc, "Checking message correct", NTPstrcmp(msgRecv, msg)==0);
	
	NTPDisconnect(&lSock);
	NTPDisconnect(&cSock);
	NTPDisconnect(&newSock);

	CuAssert(tc,"disconnect check", lSock==NULL && cSock==NULL && newSock==NULL);
}

CuSuite *getNetworkSuite(void) {
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, testDisconnectWhileConnecting);
	SUITE_ADD_TEST(suite, testConnect);
	return suite;
}
