#include <stdio.h>
#include <CuTest.h>

void testDisconnectWhileConnecting(CuTest *tc) {

}

CuSuite *getNetworkSuite(void) {
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, testDisconnectWhileConnecting);
	return suite;
}
