#include <brass.h>
#include <detect/app.h>

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>


TEST_GROUP(detect_app) {
	struct brass_net net;
	struct brass_app detect;

	void
	setup() {
        mock().expectOneCall("neighbor_discovery_open");
        mock().expectOneCall("neighbor_discovery_start")
            .withParameter("val", uint8_t(-1));
        mock().expectOneCall("unicast_open");
        mock().expectOneCall("neighbor_discovery_close");
        mock().expectOneCall("unicast_close");

		brass_app_init(&detect);
		brass_net_open(&net, 0);
		brass_net_bind(&net, &detect);
		detect.map = detect_map;
		detect.reduce = detect_reduce;
	}

	void
	teardown() {
		brass_app_cleanup(&detect);
		brass_net_close(&net);
		mock().ignoreOtherCalls();
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(detect_app, list) {
    mock().expectNCalls(1, "unicast_send");
	linkaddr_t parent = { { 1, 1 } };
	brass_net_set_parent(&net, &parent);

	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 3);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 2);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 0);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 3);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 0);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 1);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 1);

	brass_net_flush(&net, 1);
	BYTES_EQUAL(brass_app_size(&detect), 0);
}

