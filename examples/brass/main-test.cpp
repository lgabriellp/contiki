#include <CppUTest/CommandLineTestRunner.h>

IMPORT_TEST_GROUP(brass_app);
IMPORT_TEST_GROUP(brass_net);
IMPORT_TEST_GROUP(collect_app);
IMPORT_TEST_GROUP(detect_app);

int
main(int argc, char * argv[]) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
