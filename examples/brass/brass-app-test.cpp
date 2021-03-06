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
		brass_app_cleanup(&app, BRASS_FLAG_ALL);
		CHECK_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 0);
	}
};

TEST(brass_app, pair_alloc) {
	pair = brass_pair_alloc(&app, 2, 8);
	
	CHECK_EQUAL(app.ram, brass_pair_sizeof(pair));
	BYTES_EQUAL(brass_pair_len(pair), 10);
	BYTES_EQUAL(brass_pair_keylen(pair), 2);
	BYTES_EQUAL(brass_pair_valuelen(pair), 8);
	BYTES_EQUAL(pair->key, ((int8_t *)pair) + sizeof(struct brass_pair));
	BYTES_EQUAL(pair->value, pair->key + brass_pair_keylen(pair));
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_PENDING), 0);
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_URGENT), 0);

	CHECK_EQUAL(app.ram, sizeof(struct brass_pair) + brass_pair_len(pair));
	brass_pair_free(pair);
	CHECK_EQUAL(app.ram, 0);
}

TEST(brass_app, pair_set_flags) {
	pair = brass_pair_alloc(&app, 2, 8);
	
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_URGENT), 0);
	brass_pair_set_flags(pair, BRASS_FLAG_URGENT, 1);
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_URGENT), 1);
	
	brass_pair_free(pair);
}

TEST(brass_app, pair_reset_flags) {
	pair = brass_pair_alloc(&app, 2, 8);
	
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_PENDING), 0);
	brass_pair_set_flags(pair, BRASS_FLAG_PENDING, 1);
	BYTES_EQUAL(brass_pair_flags(pair, BRASS_FLAG_PENDING), 1);
	
	brass_pair_free(pair);
}

TEST(brass_app, pair_dup) {
	pair = brass_pair_alloc(&app, 2, 8);
	brass_pair_set_flags(pair, BRASS_FLAG_PENDING, 1);
	brass_pair_set_flags(pair, BRASS_FLAG_URGENT, 1);
	struct brass_pair * dup = brass_pair_dup(pair);

	BYTES_EQUAL(brass_pair_len(dup), 10);
	BYTES_EQUAL(brass_pair_keylen(dup), 2);
	BYTES_EQUAL(brass_pair_valuelen(dup), 8);
	BYTES_EQUAL(dup->key, ((int8_t *)dup) + sizeof(struct brass_pair));
	BYTES_EQUAL(dup->value, dup->key + brass_pair_keylen(dup));
	CHECK(memcmp(pair->key, dup->key, brass_pair_keylen(pair)) == 0);
	CHECK(memcmp(pair->value, dup->value, brass_pair_keylen(pair)) == 0);
	BYTES_EQUAL(brass_pair_flags(dup, BRASS_FLAG_PENDING), 1);
	BYTES_EQUAL(brass_pair_flags(dup, BRASS_FLAG_URGENT), 1);

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

	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 4);

	key = 0;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 12);
	key = 1;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 15);
	key = 2;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 8);
	key = 3;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 10);
	
	brass_app_cleanup(&app, BRASS_FLAG_ALL);
	brass_pair_free(pair);
};

TEST(brass_app, sow) {
	int8_t key = 0;
	int8_t value;

	for (value = 0; value < 10; value++) {
		brass_app_sow(&app, key, value);
	}

	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);
	
	key = 0;
	CHECK(!brass_app_find(&app, &key, sizeof(key)));
	key = 1;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 30);
	key = 2;
	CHECK(!brass_app_find(&app, &key, sizeof(key)));
	key = 3;
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 20);
	
	brass_app_cleanup(&app, BRASS_FLAG_ALL);
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

	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);
	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer), 0), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 0);

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

	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);

	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer), 0), 10);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 2);

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

TEST(brass_app, gather_urgent) {
	int8_t buffer[12];
	uint8_t key;
	pair = brass_pair_alloc(&app, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_app_emit(&app, pair);
	key = 'A';
	CHECK(!brass_pair_flags(brass_app_find(&app, &key, 1), BRASS_FLAG_URGENT));

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_pair_set_flags(pair, BRASS_FLAG_URGENT, 1);
	brass_app_emit(&app, pair);

	key = 'C';
	CHECK(brass_pair_flags(brass_app_find(&app, &key, 1), BRASS_FLAG_URGENT));
	key = 'A';
	CHECK(!brass_pair_flags(brass_app_find(&app, &key, 1), BRASS_FLAG_URGENT));
	
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);

	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer), 1), 6);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_URGENT), 1);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 1);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);

	BYTES_EQUAL(buffer[0], app.id);
	BYTES_EQUAL(buffer[1], 6);
	BYTES_EQUAL(buffer[2], brass_pair_keylen(pair));
	BYTES_EQUAL(buffer[3], brass_pair_valuelen(pair));
	BYTES_EQUAL(buffer[4], 'C');
	BYTES_EQUAL(buffer[5], 'D');
	
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
	
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);

	BYTES_EQUAL(brass_app_gather(&app, buffer, sizeof(buffer), 0), 6);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_PENDING), 1);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);

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
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_URGENT), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 0);
}

TEST(brass_app, feed_one) {
	uint8_t buffer[] = { app.id, 6, 1, 1, 'C', 'D', 0, 0 };
	uint8_t key = 'C';
	
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), 6);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 1);
	
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'D');
}

TEST(brass_app, feed_urgent) {
	uint8_t buffer[] = { app.id, 6, 1, 1, 'C', 'D', 0, 0 };
	
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 1), 6);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_URGENT), 1);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 1);
}


TEST(brass_app, feed_exact) {
	uint8_t buffer[] = { app.id, 6, 1, 1, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), sizeof(buffer));
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 1);
}

TEST(brass_app, feed_many) {
	uint8_t buffer[] = { app.id, 10, 1, 1, 'C', 'D', 1, 1, 'A', 'B', 0, 0, 0 };
	uint8_t key;
	
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), 10);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 2);
	
	key = 'C';
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'D');
	key = 'A';
	BYTES_EQUAL(brass_app_find(&app, &key, sizeof(key))->value[0], 'B');
}

TEST(brass_app, feed_wrong_id) {
	uint8_t buffer[] = { 255, 10, 1, 1, 'C', 'D', 1, 1, 'A', 'B', 0, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), 0);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 0);
}

TEST(brass_app, feed_incomplete) {
	uint8_t buffer[] = { app.id, 4, 1, 1, 0, 0 };
	BYTES_EQUAL(brass_app_feed(&app, buffer, sizeof(buffer), 0), 4);
	BYTES_EQUAL(brass_app_size(&app, BRASS_FLAG_ALL), 0);
}
