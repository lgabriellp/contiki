#include <brass.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRASS_ID_INDEX 0
#define BRASS_SIZE_INDEX 1

struct pair *
brass_pair_alloc(struct brass * brass, uint8_t keylen, uint8_t valuelen) {
	struct pair * pair = malloc(sizeof(struct pair) + keylen + valuelen);
	if (!pair) {
		printf("Failed to alloc pair\n");
		return NULL;
	}

	pair->next = NULL;
	pair->brass = brass;
	pair->key = ((int8_t *)pair) + sizeof(struct pair);
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->allocd = 1;

	return pair;
}

struct pair *
brass_pair_dup(struct pair * pair) {
	struct pair * dup = brass_pair_alloc(pair->brass, brass_pair_keylen(pair), brass_pair_valuelen(pair));
	if (!dup) {
		printf("Failed to duplicate pair\n");
		return NULL;
	}

	brass_pair_set_key(dup, pair->key);
	brass_pair_set_value(dup, pair->value);

	return dup;
}

struct pair *
brass_pair_init(struct brass * brass, struct pair * pair, void * key, uint8_t keylen, uint8_t valuelen) {
	pair->next = NULL;
	pair->brass = brass;
	pair->key = (int8_t *)key + 2;
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->allocd = 0;

	return pair;
}

void
brass_pair_free(struct pair * pair) {
	if (!pair->allocd) return;
	free(pair);
};

uint8_t
brass_pair_len(struct pair * pair) {
	return pair->len;
}

uint8_t
brass_pair_keylen(struct pair * pair) {
	return pair->value - pair->key;
}

uint8_t
brass_pair_valuelen(struct pair * pair) {
	return pair->len - brass_pair_keylen(pair);
}

int8_t
brass_pair_cmp(struct pair * pair, const void * key, uint8_t len) {
	int diff = brass_pair_keylen(pair) - len;
	return diff == 0 ? memcmp(pair->key, key, brass_pair_keylen(pair)) : diff;
}

void
brass_pair_set_key(struct pair * pair, const void * key) {
	memcpy(pair->key, key, brass_pair_keylen(pair));
}

void
brass_pair_set_value(struct pair * pair, const void * value) {
	memcpy(pair->value, value, brass_pair_valuelen(pair));
}

void
brass_pair_print(struct pair * pair) {
	int i;
	printf("pair(%d, ", pair->len);
	for (i = 0; i < pair->len; i++) {
		if (i == brass_pair_keylen(pair)) printf(", ");
		printf("%x", pair->key[i]);
	}
	printf(")\n");
}

void
brass_init(struct brass * brass) {
	LIST_STRUCT_INIT(brass, reduced);
}

void
brass_clean(struct brass * brass) {
	void * iter = list_pop(brass->reduced);

	while (iter) {
		brass_pair_free((struct pair *)iter);
		iter = list_pop(brass->reduced);
	}
}

uint8_t
brass_size(struct brass * brass) {
	return list_length(brass->reduced);
}

struct pair *
brass_find(struct brass * brass, const void * key, uint8_t len) {
	void * iter = list_head(brass->reduced);

	while (iter && brass_pair_cmp((struct pair *)iter, key, len) != 0) {
		iter = list_item_next(iter);
	}

	return iter;
}

uint8_t
brass_sow(struct brass * brass, int8_t key, int8_t value) {
	brass->map(brass, key, value);
	return 0;
}

uint8_t
brass_emit(struct brass * brass, struct pair * next) {
	struct pair * acc = brass_find(brass, next->key, brass_pair_keylen(next));
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
brass_gather(struct brass * brass, void * ptr, uint8_t len) {
	uint8_t * buf = (uint8_t *)ptr;
	uint8_t size = 0;
	uint8_t i;

	if (size + 2 >= len) return size;
	buf[BRASS_ID_INDEX] = brass->id;
	buf[BRASS_SIZE_INDEX] = 0;
	size += 2;

	struct pair * iter;
	while ((iter = (struct pair *)list_head(brass->reduced))) {
		if (size + 2 + brass_pair_len(iter) >= len) return size;

		buf[size++] = brass_pair_keylen(iter);
		buf[size++] = brass_pair_valuelen(iter);
		
		for (i = 0; i < brass_pair_len(iter); i++) {
			buf[size++] = iter->key[i];
		}
		
		buf[BRASS_SIZE_INDEX]++;
		brass_pair_free((struct pair *)list_pop(brass->reduced));
	}
	
	return size;
}

uint8_t
brass_feed(struct brass * brass, const void * ptr, uint8_t len) {
	uint8_t size = 2;
	const uint8_t * buf = (const uint8_t *)ptr;
	struct pair * pair;
	uint8_t i;

	for (i = 0; i < buf[BRASS_SIZE_INDEX]; i++) {
		pair = brass_pair_alloc(brass, buf[size], buf[size + 1]);
		size += 2;

		brass_pair_set_key(pair, &buf[size]);
		size += brass_pair_keylen(pair);

		brass_pair_set_value(pair, &buf[size]);
		size += brass_pair_valuelen(pair);

		brass_emit(brass, pair);
		brass_pair_free(pair);
	}

	return size;
}

void
brass_print(struct brass * brass) {
	void * iter = list_head(brass->reduced);

	while (iter) {
		brass_pair_print((struct pair *)iter);
		iter = list_item_next(iter);
	}	
}

