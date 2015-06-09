#include <stdio.h>
#include <CuTest.h>

CuSuite *getNetworkSuite();

//returns 1 on failure, 0 on success (like unix command line)
int runAllTests(void) {
	int rv = 0;
	CuString *output = CuStringNew();
	CuSuite *suite   = CuSuiteNew();

	CuSuiteAddSuite(suite, getNetworkSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	if(suite->failCount!=0) {
		printf("There were failures\n");
		rv = 1;
	}

	CuStringDelete(output);
	CuSuiteDelete(suite);
	return rv;
}

int main(void) {
	return runAllTests();
}
