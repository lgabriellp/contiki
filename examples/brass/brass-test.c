#include <contiki.h>
#include <unit-test.h>
#include <brass.h>

UNIT_TEST_REGISTER(brass_pair_test, "Brass Buf");
UNIT_TEST_REGISTER(brass_pair_cmp_test, "Brass Buf Compare");
UNIT_TEST_REGISTER(brass_emit_test, "Brass Emit Buf");
UNIT_TEST_REGISTER(brass_sow_test, "Brass Sow Buf");

UNIT_TEST(brass_pair_test) {
	struct pair * pair;
	UNIT_TEST_BEGIN();

	pair = brass_pair_alloc(10, 2);
	UNIT_TEST_ASSERT(brass_pair_len(pair) == 10);
	UNIT_TEST_ASSERT(brass_pair_keylen(pair) == 2);
	UNIT_TEST_ASSERT(brass_pair_valuelen(pair) == 8);
	UNIT_TEST_ASSERT(pair->key == ((int8_t *)pair) + sizeof(struct pair));
	UNIT_TEST_ASSERT(pair->value == ((int8_t *)pair) + sizeof(struct pair) + brass_pair_keylen(pair));
	brass_pair_free(pair);

	UNIT_TEST_END();
}

UNIT_TEST(brass_pair_cmp_test) {
	struct pair * pair;
	UNIT_TEST_BEGIN();

	pair = brass_pair_alloc(5, 2);
	brass_pair_set_key(pair, "00");

	UNIT_TEST_ASSERT(brass_pair_cmp(pair, "00", 2) == 0);
	UNIT_TEST_ASSERT(brass_pair_cmp(pair, "01", 2) != 0);
	UNIT_TEST_ASSERT(brass_pair_cmp(pair, "0", 1) != 0);
	UNIT_TEST_ASSERT(brass_pair_cmp(pair, "000", 3) != 0);

	brass_pair_free(pair);

	UNIT_TEST_END();
}

struct brass brass;

void
map0(int8_t key, int8_t value) {
	if (value % 2 == 0) return;

	struct pair * tmp = brass_pair_alloc(2, 1);

	tmp->key[0] = value % 4;
	tmp->value[0] = 2 * value;

	brass_emit(&brass, tmp);
	brass_pair_free(tmp);
}

void
reduce0(struct pair * acc, int8_t * next) {
	acc->value[0] += next[0];
}

UNIT_TEST(brass_emit_test) {
	struct pair pair;
	int8_t array[2];
	int8_t key;
	UNIT_TEST_BEGIN();

	brass_init(&brass);
	brass.map = map0;
	brass.reduce = reduce0;
	brass_pair_init(&pair, &array[0], &array[1], 2);

	for (array[1] = 0; array[1] < 10; array[1]++) {
		array[0] = array[1] % 4;
		brass_emit(&brass, &pair);
	}

	UNIT_TEST_ASSERT(brass_size(&brass) == 4);
	brass_print(&brass);

	key = 0;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 12);
	key = 1;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 15);
	key = 2;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 8);
	key = 3;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 10);
	
	brass_pair_free(&pair);
	brass_clean(&brass);
	UNIT_TEST_END();
}

UNIT_TEST(brass_sow_test) {
	int8_t key = 0;
	int8_t value;
	UNIT_TEST_BEGIN();

	brass_init(&brass);
	brass.map = map0;
	brass.reduce = reduce0;

	for (value = 0; value < 10; value++) {
		brass_sow(&brass, key, value);
	}

	brass_print(&brass);
	UNIT_TEST_ASSERT(brass_size(&brass) == 2);
	
	key = 0;
	UNIT_TEST_ASSERT(!brass_find(&brass, &key, sizeof(key)));
	key = 1;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 30);
	key = 2;
	UNIT_TEST_ASSERT(!brass_find(&brass, &key, sizeof(key)));
	key = 3;
	UNIT_TEST_ASSERT(brass_find(&brass, &key, sizeof(key))->value[0] == 20);


	brass_clean(&brass);
	UNIT_TEST_END();
}

PROCESS(test_process, "Unit testing");
AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data) {
	PROCESS_BEGIN();

	UNIT_TEST_RUN(brass_pair_test);
	UNIT_TEST_RUN(brass_pair_cmp_test);
	UNIT_TEST_RUN(brass_emit_test);
	UNIT_TEST_RUN(brass_sow_test);

	PROCESS_END();
}

