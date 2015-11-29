#include <brass.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRASS_DEBUG 1 
#define BRASS_ID_INDEX 0
#define BRASS_SIZE_INDEX 1

#if BRASS_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__);
#else//BRASS_DEBUG
#define PRINTF(...)
#endif//BRASS_DEBUG


struct brass_pair *
brass_pair_alloc(struct brass_app * brass, uint8_t keylen, uint8_t valuelen) {
	struct brass_pair * pair = malloc(sizeof(struct brass_pair) + keylen + valuelen);
	if (!pair) {
		PRINTF("Failed to alloc pair\n");
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
		PRINTF("Failed to duplicate pair\n");
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
	PRINTF("pair(%d, ", pair->len);
	for (i = 0; i < pair->len; i++) {
		if (i == brass_pair_keylen(pair)) {
			PRINTF(", ");
		} else {
			PRINTF(" ");
		}

		PRINTF("%x", pair->key[i]);
	}
	PRINTF(")\n");
}

void
brass_collector_init() {

}

void brass_collector_cleanup() {

}

void
brass_collector_configure(uint8_t type, clock_time_t interval, struct brass_app * app) {

}

void
brass_app_init(struct brass_app * brass) {
	LIST_STRUCT_INIT(brass, reduced);
}

void
brass_app_cleanup(struct brass_app * brass) {
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
			PRINTF("Failed to emit\n");
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

static void
neighbor_discovered(struct neighbor_discovery_conn * nd,
                    const linkaddr_t * from,
                    uint16_t hops) {
    struct brass_net * net = (struct brass_net *)
        ((char *)nd - offsetof(struct brass_net, nd));

    PRINTF("hops=%d new=%d\n", brass_net_hops(net), hops);
    if (hops >= brass_net_hops(net)) {
        return;
    }

    brass_net_set_hops(net, hops);
    brass_net_set_parent(net, from);
}

static void
neighbor_query(struct neighbor_discovery_conn * nd) {
    struct brass_net * net = (struct brass_net *)
        ((char *)nd - offsetof(struct brass_net, nd));

    net->cycles++;
}

static void
unicast_recv(struct unicast_conn * uc, const linkaddr_t *from) {
    //PRINTF("received from=%d\n", from->u8[0]);
    //mapreduce_emit_reduced(packetbuf_dataptr(), packetbuf_datalen());
}

static void
unicast_sent(struct unicast_conn * uc, int status, int num_tx) {

}

static const struct neighbor_discovery_callbacks ndc = {
    neighbor_discovered,
    neighbor_query
};

static struct unicast_callbacks ucc = {
    unicast_recv,
    unicast_sent
};

void
brass_net_open(struct brass_net * net, uint8_t is_sync) {
	LIST_STRUCT_INIT(net, apps);
    neighbor_discovery_open(&net->nd,
                            128,
                            CLOCK_SECOND * 6,
                            CLOCK_SECOND * 300UL,
                            CLOCK_SECOND * 600UL,
                            &ndc);

    unicast_open(&net->uc,
                  129,
                  &ucc);
    
    linkaddr_copy(&net->parent, &linkaddr_null);
    net->cycles = -1;
    net->hops = is_sync ? 0 : -1;
    
    neighbor_discovery_start(&net->nd, is_sync ? net->hops + 1 : net->hops);
}

void
brass_net_close(struct brass_net * net) {
	neighbor_discovery_close(&net->nd);
	unicast_close(&net->uc);
}

void
brass_net_bind(struct brass_net * net, struct brass_app * app) {
	list_add(net->apps, app);
}

void
brass_net_unbind(struct brass_net * net, struct brass_app * app) {
	list_remove(net->apps, app);
}

int
brass_net_push(struct brass_net * net) {
/*
    uint16_t datalen = PACKETBUF_SIZE;
    void * dataptr = packetbuf_dataptr();
    
    datalen = mapreduce_copy_reduced(dataptr, datalen);
    packetbuf_set_datalen(datalen);

    PRINTF("sending to=%d\n", net->parent.u8[0]);
    return unicast_send(&net->uc, &net->parent);
*/
	return 0;
}

int
brass_net_foward(struct brass_net * net, uint8_t cycle, uint8_t hops) {
    return unicast_send(&net->uc, &net->parent);
}

uint8_t
brass_net_size(const struct brass_net * net) {
	return list_length(net->apps);
}

uint8_t
brass_net_cycles(const struct brass_net * net) {
    return net->cycles;
}

uint8_t
brass_net_hops(const struct brass_net * net) {
    return net->hops;
}

const linkaddr_t *
brass_net_parent(const struct brass_net * net) {
    return &net->parent;
}

void
brass_net_set_hops(struct brass_net * net, uint8_t value) {
    net->hops = value;
    neighbor_discovery_set_val(&net->nd, value + 1);
    PRINTF("hops=%d\n", value);
}

void
brass_net_set_parent(struct brass_net * net, const linkaddr_t * value) {
    linkaddr_copy(&net->parent, value);
    PRINTF("parent=%d\n", value->u8[0]);
}
