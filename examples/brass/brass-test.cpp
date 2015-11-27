#include <brass.h>
#include <CppUTest/TestHarness.h>

TEST_GROUP(brass_pair) {
	struct pair * pair;
};

TEST(brass_pair, alloc) {
	pair = brass_pair_alloc(2, 8);
	
	CHECK(brass_pair_len(pair) == 10);
	CHECK(brass_pair_keylen(pair) == 2);
	CHECK(brass_pair_valuelen(pair) == 8);
	CHECK(pair->key == ((int8_t *)pair) + sizeof(struct pair));
	CHECK(pair->value == pair->key + brass_pair_keylen(pair));
	
	brass_pair_free(pair);
}

TEST(brass_pair, dup) {
	struct pair * dup;

	pair = brass_pair_alloc(2, 8);
	dup = brass_pair_dup(pair);
	
	CHECK(brass_pair_len(dup) == 10);
	CHECK(brass_pair_keylen(dup) == 2);
	CHECK(brass_pair_valuelen(dup) == 8);
	CHECK(dup->key == ((int8_t *)dup) + sizeof(struct pair));
	CHECK(dup->value == dup->key + brass_pair_keylen(dup));

	brass_pair_free(pair);
}


TEST(brass_pair, cmp) {
	pair = brass_pair_alloc(2, 3);
	brass_pair_set_key(pair, "00");

	CHECK(brass_pair_cmp(pair, "00", 2) == 0);
	CHECK(brass_pair_cmp(pair, "01", 2) != 0);
	CHECK(brass_pair_cmp(pair, "0", 1) != 0);
	CHECK(brass_pair_cmp(pair, "000", 3) != 0);

	brass_pair_free(pair);
}

TEST_GROUP(brass) {
	struct brass brass;
};

void
map0(struct brass * brass, int8_t key, int8_t value) {
	if (value % 2 == 0) return;

	struct pair * tmp = brass_pair_alloc(1, 1);

	tmp->key[0] = value % 4;
	tmp->value[0] = 2 * value;

	brass_emit(brass, tmp);
	brass_pair_free(tmp);
}

void
reduce0(struct pair * acc, const int8_t * next) {
	acc->value[0] += next[0];
}

TEST(brass, emit) {
	int8_t key;

	brass_init(&brass);
	brass.reduce = reduce0;
	struct pair * pair = brass_pair_alloc(1, 1);

	for (pair->value[0] = 0; pair->value[0] < 10; pair->value[0]++) {
		pair->key[0] = pair->value[0] % 4;
		brass_emit(&brass, pair);
	}

	//brass_print(&brass);
	CHECK(brass_size(&brass) == 4);

	key = 0;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 12);
	key = 1;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 15);
	key = 2;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 8);
	key = 3;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 10);
	
	brass_pair_free(pair);
	brass_clean(&brass);
};

TEST(brass, sow) {
	int8_t key = 0;
	int8_t value;

	brass_init(&brass);
	brass.map = map0;
	brass.reduce = reduce0;

	for (value = 0; value < 10; value++) {
		brass_sow(&brass, key, value);
	}

	//brass_print(&brass);
	CHECK(brass_size(&brass) == 2);
	
	key = 0;
	CHECK(!brass_find(&brass, &key, sizeof(key)));
	key = 1;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 30);
	key = 2;
	CHECK(!brass_find(&brass, &key, sizeof(key)));
	key = 3;
	CHECK(brass_find(&brass, &key, sizeof(key))->value[0] == 20);

	brass_clean(&brass);
}

