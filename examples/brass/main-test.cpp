#include <CppUTest/CommandLineTestRunner.h>

IMPORT_TEST_GROUP(brass_pair);

int
main(int argc, char * argv[]) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
