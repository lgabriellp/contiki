#include <brass.h>
#include <collect/app.h>

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(collect_app) {
	struct brass_app app;

	void
	setup() {
		node_loc_x = 10;
		node_loc_y = 20;

		brass_app_init(&app);
		app.map = collect_map;
		app.reduce = collect_reduce;
	}

	void
	teardown() {
		brass_app_cleanup(&app);
	}
};

TEST(collect_app, list) {
	brass_app_sow(&app, BRASS_SENSOR_TEMP, 3);
	brass_app_sow(&app, BRASS_SENSOR_HUMIDITY, 2);
	brass_app_sow(&app, BRASS_SENSOR_TEMP, 0);
	brass_app_sow(&app, BRASS_SENSOR_HUMIDITY, 3);
	brass_app_sow(&app, BRASS_SENSOR_HUMIDITY, 0);
	brass_app_sow(&app, BRASS_SENSOR_HUMIDITY, 1);
	brass_app_sow(&app, BRASS_SENSOR_TEMP, 1);

	brass_app_print(&app, "reduced ");
	BYTES_EQUAL(brass_app_size(&app), 2);
}

