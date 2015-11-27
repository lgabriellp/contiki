#ifndef _BRASS_H_
#define _BRASS_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include <lib/list.h>

struct brass;

struct pair {
	struct pair * next;
	struct brass * brass;
	int8_t * key;
	int8_t * value;
	uint8_t allocd;
	uint8_t len;
};

struct pair * brass_pair_alloc(struct brass * brass, uint8_t len, uint8_t key_len);
struct pair * brass_pair_dup(struct pair * pair);
void		 brass_pair_free(struct pair * pair);

uint8_t brass_pair_len(struct pair * pair);
uint8_t brass_pair_keylen(struct pair * pair);
uint8_t brass_pair_valuelen(struct pair * pair);
int8_t	brass_pair_cmp(struct pair * pair, const void * key, uint8_t len);

void brass_pair_set_key(struct pair * pair, const void * key);
void brass_pair_set_value(struct pair * pair, const void * value);
void brass_pair_print(struct pair * pair);

typedef void (*map_t)(struct brass *, int8_t key, int8_t value);
typedef void (*reduce_t)(struct pair * result, const int8_t * next);

struct brass {
	LIST_STRUCT(reduced);
	map_t map;
	reduce_t reduce;
	uint8_t id;
};

void brass_init(struct brass * brass);
void brass_clean(struct brass * brass);

struct pair * brass_find(struct brass * brass, const void * key, uint8_t len);

uint8_t brass_size(struct brass * brass);
uint8_t brass_sow(struct brass * brass, int8_t key, int8_t value);
uint8_t brass_emit(struct brass * brass, struct pair * next);
uint8_t	brass_gather(struct brass * brass, void * buf, uint8_t len);
uint8_t brass_feed(struct brass * brass, const void * buf, uint8_t len);

void brass_print(struct brass * brass);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_BRASS_H_
