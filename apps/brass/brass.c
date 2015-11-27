#include <brass.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pair *
brass_pair_alloc(uint8_t keylen, uint8_t valuelen) {
	struct pair * pair = malloc(sizeof(struct pair) + keylen + valuelen);
	if (!pair) {
		printf("Failed to alloc pair\n");
		return NULL;
	}

	pair->next = NULL;
	pair->len = keylen + valuelen;
	pair->allocd = 1;
	pair->key = ((int8_t *)pair) + sizeof(struct pair);
	pair->value = pair->key + keylen;

	return pair;
}

struct pair *
brass_pair_dup(struct pair * pair) {
	struct pair * dup = brass_pair_alloc(brass_pair_keylen(pair), brass_pair_valuelen(pair));
	if (!dup) {
		printf("Failed to duplicate pair\n");
		return NULL;
	}

	brass_pair_set_key(dup, pair->key);
	brass_pair_set_value(dup, pair->value);

	return dup;
}

void
brass_pair_init(struct pair * pair, int8_t * key, int8_t * value, uint8_t len) {
	bzero(pair, sizeof(struct pair) + len);
	pair->key = key;
	pair->value = value;
	pair->len = len;
	pair->allocd = 0;
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

int
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

void
brass_print(struct brass * brass) {
	void * iter = list_head(brass->reduced);

	while (iter) {
		brass_pair_print((struct pair *)iter);
		iter = list_item_next(iter);
	}	
}


