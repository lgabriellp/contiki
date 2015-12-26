#include <brass.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRASS_DEBUG 1
#define BRASS_ID_INDEX 0
#define BRASS_LEN_INDEX 1

#if BRASS_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__);
#else//BRASS_DEBUG
#define PRINTF(...)
#endif//BRASS_DEBUG

struct brass_pair *
brass_pair_alloc(struct brass_app * app, uint8_t keylen, uint8_t valuelen) {
	struct brass_pair * pair = malloc(sizeof(struct brass_pair) + keylen + valuelen);
	if (!pair) {
		PRINTF("Failed to alloc pair size=%d\n", sizeof(struct brass_pair) + keylen + valuelen);
		return NULL;
	}

	bzero(pair, sizeof(struct brass_pair) + keylen + valuelen);
	pair->key = ((int8_t *)pair) + sizeof(struct brass_pair);
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->flags = BRASS_FLAG_ALLOCD;
	pair->next = NULL;
	pair->app = app;
	pair->app->ram += brass_pair_sizeof(pair);
	PRINTF("alloc app=%d ram=%d\n", pair->app->id, pair->app->ram);

	return pair;
}

struct brass_pair *
brass_pair_dup(const struct brass_pair * pair) {
	struct brass_pair * dup = brass_pair_alloc(pair->app, brass_pair_keylen(pair), brass_pair_valuelen(pair));
	if (!dup) {
		PRINTF("Failed to duplicate pair\n");
		return NULL;
	}

	brass_pair_set_key(dup, pair->key);
	brass_pair_set_value(dup, pair->value);
	dup->flags = pair->flags;

	return dup;
}

void
brass_pair_free(struct brass_pair * pair) {
	if (!(pair->flags & BRASS_FLAG_ALLOCD)) return;
	pair->app->ram -= brass_pair_sizeof(pair);
	PRINTF("dealloc app=%d ram=%d\n", pair->app->id, pair->app->ram);
	free(pair);
};

struct brass_pair *
brass_pair_init(struct brass_app * app, struct brass_pair * pair, void * key, uint8_t keylen, uint8_t valuelen) {
	pair->next = NULL;
	pair->app = app;
	pair->key = (int8_t *)key + 2;
	pair->value = pair->key + keylen;
	pair->len = keylen + valuelen;
	pair->flags = 0;

	return pair;
}
uint8_t
brass_pair_sizeof(const struct brass_pair * pair) {
	return sizeof(struct brass_pair) + pair->len;
}

uint8_t
brass_pair_len(const struct brass_pair * pair) {
	return pair->len;
}

uint8_t
brass_pair_keylen(const struct brass_pair * pair) {
	return pair->value - pair->key;
}

uint8_t
brass_pair_valuelen(const struct brass_pair * pair) {
	return pair->len - brass_pair_keylen(pair);
}

uint8_t
brass_pair_flags(const struct brass_pair * pair, uint8_t flags) {
	return (pair->flags & flags) != 0;
}

int8_t
brass_pair_cmp(const struct brass_pair * pair, const void * key, uint8_t len) {
	int diff = brass_pair_keylen(pair) - len;
	return diff == 0 ? memcmp(pair->key, key, brass_pair_keylen(pair)) : diff;
}

void
brass_pair_set_flags(struct brass_pair * pair, uint8_t flags, uint8_t enabled) {
	if (enabled) pair->flags |=  flags;
	else         pair->flags &= ~flags;
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
brass_pair_print(const struct brass_pair * pair, const char * prefix) {
	int i;
	PRINTF("%spair(len=%d, ", prefix, pair->len);
   	PRINTF("urgent=%d, ", brass_pair_flags(pair, BRASS_FLAG_URGENT));
   	PRINTF("pending=%d, ", brass_pair_flags(pair, BRASS_FLAG_PENDING));

	for (i = 0; i < pair->len; i++) {
		if (i == brass_pair_keylen(pair)) {
			PRINTF(", ");
		}
		PRINTF("%2.2x", (uint8_t)pair->key[i]);
	}
	PRINTF(")\n");
}

void
brass_app_init(struct brass_app * app) {
	LIST_STRUCT_INIT(app, reduced);
	app->net = NULL;
	app->ram = 0;
}

void
brass_app_cleanup(struct brass_app * app, uint8_t filter) {
	struct brass_pair * pair = (struct brass_pair *)list_pop(app->reduced);
	struct brass_pair * temp;

	while (pair) {
		temp = pair;
		pair = (struct brass_pair *)list_item_next(pair);
		if (!brass_pair_flags(temp, filter)) continue;
		
		list_remove(app->reduced, temp);
		brass_pair_free(temp);
	}
}

uint8_t
brass_app_size(struct brass_app * app, uint8_t filter) {
	struct brass_pair * pair = (struct brass_pair *)list_head(app->reduced);
	uint8_t size = 0;

	while (pair) {
		if (brass_pair_flags(pair, filter)) size++;
		pair = (struct brass_pair *)list_item_next(pair);
	}

	return size;
}

struct brass_pair *
brass_app_find(struct brass_app * app, const void * key, uint8_t len) {
	void * iter = list_head(app->reduced);

	while (iter && brass_pair_cmp((struct brass_pair *)iter, key, len) != 0) {
		iter = list_item_next(iter);
	}

	return iter;
}

uint8_t
brass_app_sow(struct brass_app * app, int8_t key, int8_t value) {
	app->map(app, key, value);
	return 0;
}

uint8_t
brass_app_emit(struct brass_app * app, const struct brass_pair * next) {
	struct brass_pair * acc = brass_app_find(app, next->key, brass_pair_keylen(next));
	if (!acc) {
		acc = brass_pair_dup(next);
		if (!acc) {
			PRINTF("Failed to emit\n");
			return -1;
		}

		list_push(app->reduced, acc);
		return 0;
	}

	app->reduce(app, acc, next->value);
	//brass_pair_print(acc, "merged  ");

	return 1;
}

uint8_t
brass_app_gather(struct brass_app * app, void * ptr, uint8_t len, uint8_t urgent) {
	uint8_t * buf = (uint8_t *)ptr;
	uint8_t size = 0;
	uint8_t i;

	if (size + 2 >= len) return 0;
	buf[BRASS_ID_INDEX] = app->id;
	buf[BRASS_LEN_INDEX] = 0;
	size += 2;

	struct brass_pair * pair = (struct brass_pair *)list_head(app->reduced);

	while (pair) {
		if (urgent && !brass_pair_flags(pair, BRASS_FLAG_URGENT)) {
			pair = (struct brass_pair *)list_item_next(pair);
			continue;
		}

		if (size + 2 + brass_pair_len(pair) >= len) return buf[BRASS_LEN_INDEX];

		buf[size + 0] = brass_pair_keylen(pair);
		buf[size + 1] = brass_pair_valuelen(pair);
		
		for (i = 0; i < brass_pair_len(pair); i++) {
			buf[size + 2 + i] = pair->key[i];
		}
		
		size += 2 + brass_pair_len(pair);
		buf[BRASS_LEN_INDEX] = size;

		brass_pair_set_flags(pair, BRASS_FLAG_PENDING, 1);
		pair = (struct brass_pair *)list_item_next(pair);
	}
	
	return buf[BRASS_LEN_INDEX];
}

int8_t
brass_app_feed(struct brass_app * app, const void * ptr, uint8_t len, uint8_t urgent) {
	const uint8_t * buf = (const uint8_t *)ptr;
	uint8_t size = 0;
	struct brass_pair * pair;
	
	if (size + 2 > len) return len;
	PRINTF("received for_me=%d end=%d\n", buf[BRASS_ID_INDEX] == app->id, buf[BRASS_LEN_INDEX]);
	if (buf[BRASS_ID_INDEX] != app->id) return 0;
	if (buf[BRASS_LEN_INDEX] > len) return len;
	size += 2;

	while(size < buf[BRASS_LEN_INDEX]) {
		if (size + 2 + buf[size] + buf[size+1] > buf[BRASS_LEN_INDEX]) return buf[BRASS_LEN_INDEX];
		pair = brass_pair_alloc(app, buf[size], buf[size + 1]);
		size += 2;
		
		brass_pair_set_flags(pair, BRASS_FLAG_URGENT, urgent);
		
		brass_pair_set_key(pair, &buf[size]);
		size += brass_pair_keylen(pair);

		brass_pair_set_value(pair, &buf[size]);
		size += brass_pair_valuelen(pair);

		brass_pair_print(pair, "feeded  ");
		brass_app_emit(app, pair);
		brass_pair_free(pair);
	}

	return size;
}

void
brass_app_print(const struct brass_app * app, const char * prefix) {
	void * iter = list_head(app->reduced);

	while (iter) {
		brass_pair_print((struct brass_pair *)iter, prefix);
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
runicast_recv(struct runicast_conn * uc, const linkaddr_t * from, uint8_t seqno) {
    struct brass_net * net = (struct brass_net *)
        ((char *)uc - offsetof(struct brass_net, uc));
    PRINTF("(%2.d,%2.d) recv msgs=%d seqno=%d len=%d\n", from->u8[0], linkaddr_node_addr.u8[0], ++net->msgs_recv, seqno, packetbuf_datalen());
	struct brass_app * app = (struct brass_app *)list_head(net->apps);
	uint8_t * buf = (uint8_t *)packetbuf_dataptr();
	uint8_t urgent = buf[0];
	uint8_t apps = buf[1];

	while (app) {
		uint8_t len = 2;
		PRINTF("runicast_recv 1\n");
		while (apps-- && len < packetbuf_datalen() && (buf[len + BRASS_ID_INDEX] != app->id)) len += buf[len + BRASS_LEN_INDEX];
		PRINTF("runicast_recv 2\n");
		brass_app_feed(app, buf + len, packetbuf_datalen() - len, urgent);
		PRINTF("runicast_recv 3\n");
		app = (struct brass_app *)list_item_next(app);
	}

	brass_net_flush(net, 1);
}

static void
runicast_sent(struct runicast_conn * uc, const linkaddr_t * to, uint8_t retransmissions) {
    PRINTF("(%2.d,%2.d) sent retx=%d\n", linkaddr_node_addr.u8[0], to->u8[0], retransmissions);
    struct brass_net * net = (struct brass_net *)
        ((char *)uc - offsetof(struct brass_net, uc));
	struct brass_app * app = (struct brass_app *)list_head(net->apps);

	while (app) {
		brass_app_cleanup(app, BRASS_FLAG_PENDING);
		PRINTF("cleanup node=%d ram=%d\n", linkaddr_node_addr.u8[0], app->ram);
		app = (struct brass_app *)list_item_next(app);
	}
}

static void
runicast_timedout(struct runicast_conn *uc, const linkaddr_t * to, uint8_t retransmissions) {
    PRINTF("(%2.d,%2.d) timedout retx=%d\n", linkaddr_node_addr.u8[0], to->u8[0], retransmissions);
}

static const struct neighbor_discovery_callbacks ndc = {
    neighbor_discovered,
    neighbor_query
};

static struct runicast_callbacks ucc = {
    runicast_recv,
    runicast_sent,
	runicast_timedout,
};

void
brass_net_open(struct brass_net * net, uint8_t is_sync) {
	LIST_STRUCT_INIT(net, apps);
    neighbor_discovery_open(&net->nd,
                            128,
                            CLOCK_SECOND * 1UL,
                            CLOCK_SECOND * 900UL,
                            CLOCK_SECOND * 900UL,
                            &ndc);

    runicast_open(&net->uc, 129, &ucc);
    
    linkaddr_copy(&net->parent, &linkaddr_null);
    net->cycles = -1;
    net->hops = is_sync ? 0 : -1;
	net->msgs_sent = 0;
	net->msgs_recv = 0;
    
    neighbor_discovery_start(&net->nd, is_sync ? net->hops + 1 : net->hops);
}

void
brass_net_close(struct brass_net * net) {
	neighbor_discovery_close(&net->nd);
	runicast_close(&net->uc);
}

void
brass_net_bind(struct brass_net * net, struct brass_app * app) {
	list_add(net->apps, app);
	app->net = net;
}

void
brass_net_unbind(struct brass_net * net, struct brass_app * app) {
	list_remove(net->apps, app);
	app->net = NULL;
}

int
brass_net_flush(struct brass_net * net, uint8_t urgent) {
	if (linkaddr_cmp(&net->parent, &linkaddr_null)) {
		PRINTF("flush no parent urgent=%d\n", urgent);
		return 0;
	}

	if (runicast_is_transmitting(&net->uc)) {
		PRINTF("flush no medium urgent=%d\n", urgent);
		return 0;
	}

	struct channel * channel = channel_lookup(129);
	uint8_t hdrlen = (channel ? channel->hdrsize : 0);
	uint8_t len = 2 + hdrlen;
	uint8_t * ptr = (uint8_t *)packetbuf_dataptr();
	struct brass_app * app = (struct brass_app *)list_head(net->apps);
	ptr[0] = urgent;
	ptr[1] = 0;
	
	while (app) {
		uint8_t size = brass_app_gather(app, ptr + len, PACKETBUF_SIZE - len, urgent);
		if (size) ptr[1]++;
		len += size;
		app = (struct brass_app *)list_item_next(app);
	}
	
	if (len <= 4 + hdrlen) {
		PRINTF("flush no data urgent=%d\n", urgent);
		return 0;
	}

	packetbuf_set_datalen(len);
    PRINTF("(%2.d,%2.d) send msgs=%d urgent=%d len=%d\n", linkaddr_node_addr.u8[0], net->parent.u8[0], ++net->msgs_sent, urgent, packetbuf_datalen());
	
    return runicast_send(&net->uc, &net->parent, 3);
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
