#include <brass.h>
#include <CppUTest/TestHarness.h>

void
map0(struct brass * brass, int8_t key, int8_t value) {
	if (value % 2 == 0) return;

	struct pair * tmp = brass_pair_alloc(brass, 1, 1);

	tmp->key[0] = value % 4;
	tmp->value[0] = 2 * value;

	brass_emit(brass, tmp);
	brass_pair_free(tmp);
}

void
reduce0(struct pair * acc, const int8_t * next) {
	acc->value[0] += next[0];
}

TEST_GROUP(brass) {
	struct brass brass;
	struct pair * pair;

	void
	setup() {
		brass_init(&brass);
		brass.map = map0;
		brass.reduce = reduce0;
	}

	void
	teardown() {
		brass_clean(&brass);
	}
};

TEST(brass, pair_alloc) {
	pair = brass_pair_alloc(&brass, 2, 8);
	
	BYTES_EQUAL(brass_pair_len(pair), 10);
	BYTES_EQUAL(brass_pair_keylen(pair), 2);
	BYTES_EQUAL(brass_pair_valuelen(pair), 8);
	BYTES_EQUAL(pair->key, ((int8_t *)pair) + sizeof(struct pair));
	BYTES_EQUAL(pair->value, pair->key + brass_pair_keylen(pair));
	
	brass_pair_free(pair);
}

TEST(brass, pair_dup) {
	pair = brass_pair_alloc(&brass, 2, 8);
	struct pair * dup = brass_pair_dup(pair);
	
	BYTES_EQUAL(brass_pair_len(dup), 10);
	BYTES_EQUAL(brass_pair_keylen(dup), 2);
	BYTES_EQUAL(brass_pair_valuelen(dup), 8);
	BYTES_EQUAL(dup->key, ((int8_t *)dup) + sizeof(struct pair));
	BYTES_EQUAL(dup->value, dup->key + brass_pair_keylen(dup));

	brass_pair_free(pair);
}

TEST(brass, pair_cmp) {
	pair = brass_pair_alloc(&brass, 2, 3);
	brass_pair_set_key(pair, "00");

	BYTES_EQUAL(brass_pair_cmp(pair, "00", 2), 0);
	BYTES_EQUAL(brass_pair_cmp(pair, "01", 2), -1);
	BYTES_EQUAL(brass_pair_cmp(pair, "0", 1), 1);
	BYTES_EQUAL(brass_pair_cmp(pair, "000", 3), -1);

	brass_pair_free(pair);
}

TEST(brass, emit) {
	int8_t key;
	struct pair * pair = brass_pair_alloc(&brass, 1, 1);

	for (pair->value[0] = 0; pair->value[0] < 10; pair->value[0]++) {
		pair->key[0] = pair->value[0] % 4;
		brass_emit(&brass, pair);
	}

	//brass_print(&brass);
	BYTES_EQUAL(brass_size(&brass), 4);

	key = 0;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 12);
	key = 1;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 15);
	key = 2;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 8);
	key = 3;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 10);
	
	brass_pair_free(pair);
};

TEST(brass, sow) {
	int8_t key = 0;
	int8_t value;

	for (value = 0; value < 10; value++) {
		brass_sow(&brass, key, value);
	}

	//brass_print(&brass);
	BYTES_EQUAL(brass_size(&brass), 2);
	
	key = 0;
	CHECK(!brass_find(&brass, &key, sizeof(key)));
	key = 1;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 30);
	key = 2;
	CHECK(!brass_find(&brass, &key, sizeof(key)));
	key = 3;
	BYTES_EQUAL(brass_find(&brass, &key, sizeof(key))->value[0], 20);
}

TEST(brass, gather_complete) {
	int8_t buffer[12];
	pair = brass_pair_alloc(&brass, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_emit(&brass, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_emit(&brass, pair);

	brass_print(&brass);
	BYTES_EQUAL(brass_gather(&brass, buffer, sizeof(buffer)), 10);
	BYTES_EQUAL(brass_size(&brass), 0);

	BYTES_EQUAL(buffer[0], brass.id);
	BYTES_EQUAL(buffer[1], 2);
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

TEST(brass, gather_incomplete) {
	int8_t buffer[8];
	pair = brass_pair_alloc(&brass, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_emit(&brass, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_emit(&brass, pair);

	brass_print(&brass);
	BYTES_EQUAL(brass_gather(&brass, buffer, sizeof(buffer)), 6);
	BYTES_EQUAL(brass_size(&brass), 1);

	BYTES_EQUAL(buffer[0], brass.id);
	BYTES_EQUAL(buffer[1], 1);
	BYTES_EQUAL(buffer[2], brass_pair_keylen(pair));
	BYTES_EQUAL(buffer[3], brass_pair_valuelen(pair));
	BYTES_EQUAL(buffer[4], 'C');
	BYTES_EQUAL(buffer[5], 'D');

	brass_pair_free(pair);
}

IGNORE_TEST(brass, gather_none) {
	int8_t buffer[0];
	pair = brass_pair_alloc(&brass, 1, 1);

	brass_pair_set_key(pair, "A");
	brass_pair_set_value(pair, "B");
	brass_emit(&brass, pair);

	brass_pair_set_key(pair, "C");
	brass_pair_set_value(pair, "D");
	brass_emit(&brass, pair);

	brass_print(&brass);
	BYTES_EQUAL(brass_gather(&brass, buffer, sizeof(buffer)), 0);
	BYTES_EQUAL(brass_size(&brass), 2);

	brass_pair_free(pair);
}
