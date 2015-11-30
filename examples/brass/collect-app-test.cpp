#include <brass.h>
#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#define TEMP_EVENT 1
#define HUMIDITY_EVENT 2

const uint8_t node_loc_x = 20;
const uint8_t node_loc_y = 10;

typedef struct __attribute__ ((packed)) collect_key {
	unsigned date:16;
	unsigned pos_x:8;
	unsigned pos_y:8;
	unsigned type:8;
} collect_key_t;

typedef struct __attribute__ ((packed)) collect_value {
	unsigned sqr_sum:16;
	unsigned sum:8;
	unsigned count:8;
} collect_value_t;

static void
map(struct brass_app * app, int8_t type, int8_t value) {
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(collect_key_t), sizeof(collect_value_t));
	collect_key_t ckey;
	collect_value_t cvalue;

	ckey.date = clock_seconds();
	ckey.pos_x = node_loc_x;
	ckey.pos_y = node_loc_y;
	ckey.type = type;

	cvalue.sqr_sum = value * value;
	cvalue.sum = value;
	cvalue.count = 1;
	
	brass_pair_set_key(pair, &ckey);
	brass_pair_set_value(pair, &cvalue);

	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

static void
reduce(struct brass_pair * acc, const int8_t * next) {
	collect_value_t * cacc = (collect_value_t *)acc->value;
	collect_value_t * cnext = (collect_value_t *)next;

	cacc->sqr_sum += cnext->sqr_sum;
	cacc->sum += cnext->sum;
	cacc->count += cnext->count;
}

TEST_GROUP(collect_app) {
	struct brass_app collect;

	void
	setup() {
		collect.map = map;
		collect.reduce = reduce;
		brass_app_init(&collect);
	}

	void
	teardown() {
		brass_app_cleanup(&collect);
	}
};

TEST(collect_app, list) {
	brass_app_sow(&collect, TEMP_EVENT, 3);
	brass_app_sow(&collect, HUMIDITY_EVENT, 2);
	brass_app_sow(&collect, TEMP_EVENT, 0);
	brass_app_sow(&collect, HUMIDITY_EVENT, 3);
	brass_app_sow(&collect, HUMIDITY_EVENT, 0);
	brass_app_sow(&collect, HUMIDITY_EVENT, 1);
	brass_app_sow(&collect, TEMP_EVENT, 1);

	brass_app_print(&collect);
	BYTES_EQUAL(brass_app_size(&collect), 2);
}

