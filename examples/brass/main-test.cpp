#include <CppUTest/CommandLineTestRunner.h>

IMPORT_TEST_GROUP(brass);

int
main(int argc, char * argv[]) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
