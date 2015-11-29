#include <brass.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRASS_ID_INDEX 0
#define BRASS_SIZE_INDEX 1

struct brass_pair *
brass_pair_alloc(struct brass_app * brass, uint8_t keylen, uint8_t valuelen) {
	struct brass_pair * pair = malloc(sizeof(struct brass_pair) + keylen + valuelen);
	if (!pair) {
		printf("Failed to alloc pair\n");
		return NULL;
	}

	pair->next = NULL;
	pair->brass = brass;
	pair->key = ((int8_t *)pair) + sizeof(struct brass_pair);
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->allocd = 1;

	return pair;
}

struct brass_pair *
brass_pair_dup(struct brass_pair * pair) {
	struct brass_pair * dup = brass_pair_alloc(pair->brass, brass_pair_keylen(pair), brass_pair_valuelen(pair));
	if (!dup) {
		printf("Failed to duplicate pair\n");
		return NULL;
	}

	brass_pair_set_key(dup, pair->key);
	brass_pair_set_value(dup, pair->value);

	return dup;
}

struct brass_pair *
brass_pair_init(struct brass_app * brass, struct brass_pair * pair, void * key, uint8_t keylen, uint8_t valuelen) {
	pair->next = NULL;
	pair->brass = brass;
	pair->key = (int8_t *)key + 2;
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->allocd = 0;

	return pair;
}

void
brass_pair_free(struct brass_pair * pair) {
	if (!pair->allocd) return;
	free(pair);
};

uint8_t
brass_pair_len(struct brass_pair * pair) {
	return pair->len;
}

uint8_t
brass_pair_keylen(struct brass_pair * pair) {
	return pair->value - pair->key;
}

uint8_t
brass_pair_valuelen(struct brass_pair * pair) {
	return pair->len - brass_pair_keylen(pair);
}

int8_t
brass_pair_cmp(struct brass_pair * pair, const void * key, uint8_t len) {
	int diff = brass_pair_keylen(pair) - len;
	return diff == 0 ? memcmp(pair->key, key, brass_pair_keylen(pair)) : diff;
}

void
brass_pair_set_key(struct brass_pair * pair, const void * key) {
	memcpy(pair->key, key, brass_pair_keylen(pair));
}

void
brass_pair_set_value(struct brass_pair * pair, const void * value) {
	memcpy(pair->value, value, brass_pair_valuelen(pair));
}

void
brass_pair_print(struct brass_pair * pair) {
	int i;
	printf("pair(%d, ", pair->len);
	for (i = 0; i < pair->len; i++) {
		if (i == brass_pair_keylen(pair)) printf(", ");
		printf("%x", pair->key[i]);
	}
	printf(")\n");
}

void
brass_app_init(struct brass_app * brass) {
	LIST_STRUCT_INIT(brass, reduced);
}

void
brass_app_clean(struct brass_app * brass) {
	void * iter = list_pop(brass->reduced);

	while (iter) {
		brass_pair_free((struct brass_pair *)iter);
		iter = list_pop(brass->reduced);
	}
}

uint8_t
brass_app_size(struct brass_app * brass) {
	return list_length(brass->reduced);
}

struct brass_pair *
brass_app_find(struct brass_app * brass, const void * key, uint8_t len) {
	void * iter = list_head(brass->reduced);

	while (iter && brass_pair_cmp((struct brass_pair *)iter, key, len) != 0) {
		iter = list_item_next(iter);
	}

	return iter;
}

uint8_t
brass_app_sow(struct brass_app * brass, int8_t key, int8_t value) {
	brass->map(brass, key, value);
	return 0;
}

uint8_t
brass_app_emit(struct brass_app * brass, struct brass_pair * next) {
	struct brass_pair * acc = brass_app_find(brass, next->key, brass_pair_keylen(next));
	if (!acc) {
		acc = brass_pair_dup(next);
		if (!acc) {
			printf("Failed to emit\n");
			return -1;
		}

		list_push(brass->reduced, acc);
		return 0;
	}

	brass->reduce(acc, next->value);

	return 1;
}

uint8_t
brass_app_gather(struct brass_app * brass, void * ptr, uint8_t len) {
	uint8_t * buf = (uint8_t *)ptr;
	uint8_t size = 0;
	uint8_t i;

	if (size + 2 >= len) return size;
	buf[BRASS_ID_INDEX] = brass->id;
	buf[BRASS_SIZE_INDEX] = 0;
	size += 2;

	struct brass_pair * iter;
	while ((iter = (struct brass_pair *)list_head(brass->reduced))) {
		if (size + 2 + brass_pair_len(iter) >= len) return size;

		buf[size++] = brass_pair_keylen(iter);
		buf[size++] = brass_pair_valuelen(iter);
		
		for (i = 0; i < brass_pair_len(iter); i++) {
			buf[size++] = iter->key[i];
		}
		
		buf[BRASS_SIZE_INDEX]++;
		brass_pair_free((struct brass_pair *)list_pop(brass->reduced));
	}
	
	return size;
}

uint8_t
brass_app_feed(struct brass_app * brass, const void * ptr, uint8_t len) {
	const uint8_t * buf = (const uint8_t *)ptr;
	uint8_t size = 0;
	struct brass_pair * pair;
	uint8_t i;
	
	if (size + 2 >= len) return len;
	if (buf[BRASS_ID_INDEX] != brass->id) return 0;
	size += 2;

	for (i = 0; i < buf[BRASS_SIZE_INDEX]; i++) {
		if (size + 2 >= len) return len;
		pair = brass_pair_alloc(brass, buf[size], buf[size + 1]);
		size += 2;

		if (size + brass_pair_len(pair) >= len) return len;
		brass_pair_set_key(pair, &buf[size]);
		size += brass_pair_keylen(pair);

		brass_pair_set_value(pair, &buf[size]);
		size += brass_pair_valuelen(pair);
		if (size >= len) return len;

		brass_app_emit(brass, pair);
		brass_pair_free(pair);
	}

	return size;
}

void
brass_app_print(struct brass_app * brass) {
	void * iter = list_head(brass->reduced);

	while (iter) {
		brass_pair_print((struct brass_pair *)iter);
		iter = list_item_next(iter);
	}	
}

