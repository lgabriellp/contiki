#include <brass.h>
#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#define PRESENCE_DETECTED_EVENT 1

typedef struct __attribute__((packed)) {
	unsigned date:16;
	unsigned animal_id:8;
} detect_key_t;

static void
map(struct brass_app * app, int8_t type, int8_t value) {
	if (!value) return;

	struct brass_pair * pair = brass_pair_alloc(app, sizeof(detect_key_t), 0);
	detect_key_t key;

	key.animal_id = value;
	key.date = clock_seconds();
	memcpy(pair->key, &key, sizeof(key));
	brass_app_emit(app, pair);
	brass_app_flush(app);
	brass_pair_free(pair);
}

static void
reduce(struct brass_pair * acc, const int8_t * next) {

}

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
		detect.map = map;
		detect.reduce = reduce;
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
	mock().expectNCalls(5, "unicast_send");
	linkaddr_t parent = { { 1, 1 } };
	brass_net_set_parent(&net, &parent);

	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 3);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 2);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 0);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 3);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 0);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 1);
	brass_app_sow(&detect, PRESENCE_DETECTED_EVENT, 1);

	brass_app_print(&detect);
	BYTES_EQUAL(brass_app_size(&detect), 0);
}

