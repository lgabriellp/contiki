#ifndef _BRASS_H_
#define _BRASS_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include <lib/list.h>

#include <net/rime/rime.h>
#include <net/rime/neighbor-discovery.h>
#include <net/rime/unicast.h>

#include <sys/timer.h>

#define BRASS_SENSOR_TEMP 1
#define BRASS_SENSOR_HUMIDITY 2

struct brass_app;
struct brass_pair;

struct brass_net {
	LIST_STRUCT(apps);
    struct neighbor_discovery_conn nd;
    struct unicast_conn uc;
    linkaddr_t parent;
    uint8_t cycles;
    uint8_t hops;
};

void brass_net_open(struct brass_net * net, uint8_t is_sink);
void brass_net_close(struct brass_net * net);
void brass_net_bind(struct brass_net * net, struct brass_app * app);
void brass_net_unbind(struct brass_net * net, struct brass_app * app);

int brass_net_flush(struct brass_net * net, uint8_t urgent);

uint8_t brass_net_size(const struct brass_net * net);
uint8_t brass_net_cycles(const struct brass_net * net);
uint8_t brass_net_hops(const struct brass_net * net);
const linkaddr_t * brass_net_parent(const struct brass_net * net);

void brass_net_set_hops(struct brass_net * net, uint8_t value);
void brass_net_set_parent(struct brass_net * net, const linkaddr_t * value);

void brass_collector_init();
void brass_collector_cleanup();
void brass_collector_configure(uint8_t type, clock_time_t interval, struct brass_app * app);

typedef void (*map_t)(struct brass_app *, int8_t key, int8_t value);
typedef void (*reduce_t)(struct brass_app *, struct brass_pair * result, const int8_t * next);

struct brass_app {
	struct brass_app * next;
	struct brass_net * net;
	LIST_STRUCT(reduced);
	map_t map;
	reduce_t reduce;
	uint8_t id;
};

void brass_app_init(struct brass_app * app);
void brass_app_cleanup(struct brass_app * app);

struct brass_pair * brass_app_find(struct brass_app * app, const void * key, uint8_t len);
void   brass_app_collect(struct brass_app * app, int8_t key, clock_time_t interval);

uint8_t brass_app_size(struct brass_app * app);
uint8_t brass_app_sow(struct brass_app * app, int8_t key, int8_t value);
uint8_t brass_app_emit(struct brass_app * app, const struct brass_pair * next);
uint8_t	brass_app_gather(struct brass_app * app, void * buf, uint8_t len, uint8_t urgent);
int8_t  brass_app_feed(struct brass_app * app, const void * buf, uint8_t len);

void brass_app_print(struct brass_app * app);

struct brass_pair {
	struct brass_pair * next;
	struct brass_app * app;
	int8_t * key;
	int8_t * value;
	uint8_t flags;
	uint8_t len;
};

struct brass_pair * brass_pair_alloc(struct brass_app * app, uint8_t len, uint8_t key_len);
struct brass_pair * brass_pair_dup(const struct brass_pair * pair);
void                brass_pair_free(struct brass_pair * pair);

uint8_t brass_pair_len(const struct brass_pair * pair);
uint8_t brass_pair_keylen(const struct brass_pair * pair);
uint8_t brass_pair_valuelen(const struct brass_pair * pair);
uint8_t brass_pair_urgent(const struct brass_pair * pair);
int8_t  brass_pair_cmp(const struct brass_pair * pair, const void * key, uint8_t len);
void    brass_pair_print(const struct brass_pair * pair, const char * prefix);

void brass_pair_set_urgent(struct brass_pair * pair, int urgent);
void brass_pair_set_key(struct brass_pair * pair, const void * key);
void brass_pair_set_value(struct brass_pair * pair, const void * value);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_BRASS_H_
