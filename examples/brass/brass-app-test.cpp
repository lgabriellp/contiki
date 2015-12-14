#include <brass.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

void
map0(struct brass_app * app, int8_t key, int8_t value) {
	if (value % 2 == 0) return;

	struct brass_pair * tmp = brass_pair_alloc(app, 1, 1);

	tmp->key[0] = value % 4;
	tmp->value[0] = 2 * value;

	brass_app_emit(app, tmp);
	brass_pair_free(tmp);
}

void
reduce0(struct brass_app * app, struct brass_pair * acc, const int8_t * next) {
	acc->value[0] += next[0];
}

TEST_GROUP(brass_app) {
	struct brass_app app;
	struct brass_pair * pair;

	void
	setup() {
		brass_app_init(&app);
		app.map = map0;
		app.reduce = reduce0;
	}

	void
	teardown() {
		brass_app_cleanup(&app);
	}
};

TEST(brass_app, pair_alloc) {
	pair = brass_pair_alloc(&app, 2, 8);
	
	BYTES_EQUAL(brass_pair_len(pair), 10);
	BYTES_EQUAL(brass_pair_keylen(pair), 2);
	BYTES_EQUAL(brass_pair_valuelen(pair), 8);
	BYTES_EQUAL(pair->key, ((int8_t *)pair) + sizeof(struct brass_pair));
	BYTES_EQUAL(pair->value, pair->key + brass_pair_keylen(pair));
	
	brass_pair_free(pair);
}

TEST(brass_app, pair_dup) {
	pair = brass_pair_alloc(&app, 2, 8);
	struct brass_pair * dup = brass_pair_dup(pair);
	
	BYTES_EQUAL(brass_pair_len(dup), 10);
	BYTES_EQUAL(brass_pair_keylen(dup), 2);
	BYTES_EQUAL(brass_pair_valuelen(dup), 8);
	BYTES_EQUAL(dup->key, ((int8_t *)dup) + sizeof(struct brass_pair));
	BYTES_EQUAL(dup->value, dup->key + brass_pair_keylen(dup));

	brass_pair_free(pair);
	brass_pair_free(dup);
}

TEST(brass_app, pair_cmp) {
	pair = brass_pair_alloc(&app, 2, 3);
	brass_pair_set_key(pair, "00");

	BYTES_EQUAL(brass_pair_cmp(pair, "00", 2), 0);
	BYTES_EQUAL(brass_pair_cmp(pair, "01", 2), -1);
	BYTES_EQUAL(brass_pair_cmp(pair, "0", 1), 1);
	BYTES_EQUAL(brass_pair_cmp(pair, "000", 3), -1);

	brass_pair_free(pair);
}

TEST(brass_app, emit) {
	int8_t key;
	struct brass_pair * pair = brass_pair_alloc(&app, 1, 1);

	for (pair->value[0] = 0; pair->value[0] < 10; pair->value[0]++) {
		pair->key[0] = pair->value[0] % 4;
		brass_app_emit(&app, pair);
	}

	//brass_print(&app);
	BYTES_EQUAL(brass_app_size(&app), 4);

	key = 0;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 12);
	key = 1;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 15);
	key = 2;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 8);
	key = 3;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 10);
	
	brass_pair_free(pair);
};

TEST(brass_app, sow) {
	int8_t key = 0;
	int8_t value;

	for (value = 0; value < 10; value++) {
		brass_app_sow(&app, key, value);
	}

	//brass_print(&app);
	BYTES_EQUAL(brass_app_size(&app), 2);
	
	key = 0;
	CHECK(!brass_app_find(&app, &key, sizeof(key)));
	key = 1;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 30);
	key = 2;
	CHECK(!brass_app_find(&app, &key, sizeof(key)));
	key = 3;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 20);
}

TEST(brass_app, gather_none) {
	int8_t buffer[0];
	pair = brass_pair_alloc(&app, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_app_emit(&app, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_app_emit(&app, pair);

	//brass_print(&app);
	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer)), 0);
	BYTES_EQUAL(brass_app_size(&app), 2);

	brass_pair_free(pair);
}


TEST(brass_app, gather_complete) {
	int8_t buffer[12];
	pair = brass_pair_alloc(&app, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_app_emit(&app, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_app_emit(&app, pair);

	//brass_print(&app);
	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer)), 10);
	BYTES_EQUAL(brass_app_size(&app), 0);

	BYTES_EQUAL(buffer[0], app.id);
	BYTES_EQUAL(buffer[1], 10);
	BYTES_EQUAL(buffer[2], brass_pair_keylen(pair));
	BYTES_EQUAL(buffer[3], brass_pair_valuelen(pair));
	BYTES_EQUAL(buffer[4], 'C');
	BYTES_EQUAL(buffer[5], 'D');
	BYTES_EQUAL(buffer[6], brass_pair_keylen(pair));
	BYTES_EQUAL(buffer[7], brass_pair_valuelen(pair));
	BYTES_EQUAL(buffer[8], 'A');
	BYTES_EQUAL(buffer[9], 'B');
	
	brass_pair_free(pair);
}

TEST(brass_app, gather_incomplete) {
	int8_t buffer[8];
	pair = brass_pair_alloc(&app, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_app_emit(&app, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_app_emit(&app, pair);

	//brass_print(&app);
	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer)), 6);
	BYTES_EQUAL(brass_app_size(&app), 1);

	BYTES_EQUAL(buffer[0], app.id);
	BYTES_EQUAL(buffer[1], 6);
	BYTES_EQUAL(buffer[2], brass_pair_keylen(pair));
	BYTES_EQUAL(buffer[3], brass_pair_valuelen(pair));
	BYTES_EQUAL(buffer[4], 'C');
	BYTES_EQUAL(buffer[5], 'D');

	brass_pair_free(pair);
}

TEST(brass_app, feed_none) {
	uint8_t buffer[] = {};
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), 0);
	BYTES_EQUAL(brass_app_size(&app), 0);
}

TEST(brass_app, feed_one) {
	uint8_t buffer[] = { app.id, 6, 1, 1, 'C', 'D', 0, 0 };
	uint8_t key = 'C';
	
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), 6);
	BYTES_EQUAL(brass_app_size(&app), 1);
	
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'D');
}

TEST(brass_app, feed_exact) {
	uint8_t buffer[] = { app.id, 6, 1, 1, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), sizeof(buffer));
	BYTES_EQUAL(brass_app_size(&app), 1);
}

TEST(brass_app, feed_many) {
	uint8_t buffer[] = { app.id, 10, 1, 1, 'C', 'D', 1, 1, 'A', 'B', 0, 0, 0 };
	uint8_t key;
	
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), 10);
	BYTES_EQUAL(brass_app_size(&app), 2);
	
	key = 'C';
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'D');
	key = 'A';
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'B');
}

TEST(brass_app, feed_wrong_id) {
	uint8_t buffer[] = { 255, 10, 1, 1, 'C', 'D', 1, 1, 'A', 'B', 0, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), 0);
	BYTES_EQUAL(brass_app_size(&app), 0);
}

TEST(brass_app, feed_incomplete) {
	uint8_t buffer[] = { app.id, 4, 1, 1, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer)), 4);
	BYTES_EQUAL(brass_app_size(&app), 0);
}
