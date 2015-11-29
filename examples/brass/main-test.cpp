#include <CppUTest/CommandLineTestRunner.h>

IMPORT_TEST_GROUP(brass_app);
IMPORT_TEST_GROUP(brass_net);

int
main(int argc, char * argv[]) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
