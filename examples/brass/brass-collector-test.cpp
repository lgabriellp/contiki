#include <brass.h>
#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(brass_collector) {
	struct brass_app app;

	void
	setup() {
		brass_app_init(&app);
		brass_collector_init();
	}

	void
	teardown() {
		brass_app_cleanup(&app);
		brass_collector_cleanup();
	}
};

TEST(brass_collector, configure_sensing_interval) {
	brass_collector_configure(1, CLOCK_SECOND, &app);
}

