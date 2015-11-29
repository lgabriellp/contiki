#ifndef _BRASS_H_
#define _BRASS_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include <lib/list.h>

struct brass_app;
struct brass_pair;
struct brass_net;

typedef void (*map_t)(struct brass_app *, int8_t key, int8_t value);
typedef void (*reduce_t)(struct brass_pair * result, const int8_t * next);

struct brass_app {
	LIST_STRUCT(reduced);
	map_t map;
	reduce_t reduce;
	uint8_t id;
};

void brass_app_init(struct brass_app * brass);
void brass_app_clean(struct brass_app * brass);

struct brass_pair * brass_app_find(struct brass_app * brass, const void * key, uint8_t len);

uint8_t brass_app_size(struct brass_app * brass);
uint8_t brass_app_sow(struct brass_app * brass, int8_t key, int8_t value);
uint8_t brass_app_emit(struct brass_app * brass, struct brass_pair * next);
uint8_t	brass_app_gather(struct brass_app * brass, void * buf, uint8_t len);
uint8_t brass_app_feed(struct brass_app * brass, const void * buf, uint8_t len);

void brass_app_print(struct brass_app * brass);

struct brass_pair {
	struct brass_pair * next;
	struct brass_app * brass;
	int8_t * key;
	int8_t * value;
	uint8_t allocd;
	uint8_t len;
};

struct brass_pair * brass_pair_alloc(struct brass_app * brass, uint8_t len, uint8_t key_len);
struct brass_pair * brass_pair_dup(struct brass_pair * pair);
void		 brass_pair_free(struct brass_pair * pair);

uint8_t brass_pair_len(struct brass_pair * pair);
uint8_t brass_pair_keylen(struct brass_pair * pair);
uint8_t brass_pair_valuelen(struct brass_pair * pair);
int8_t	brass_pair_cmp(struct brass_pair * pair, const void * key, uint8_t len);

void brass_pair_set_key(struct brass_pair * pair, const void * key);
void brass_pair_set_value(struct brass_pair * pair, const void * value);
void brass_pair_print(struct brass_pair * pair);

//void brass_net_init(struct brass)

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_BRASS_H_
