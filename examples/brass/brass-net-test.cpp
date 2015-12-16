#include <brass.h>
#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct neighbor_discovery_callbacks fake_ndc;
struct runicast_callbacks fake_ucc;
int fake_rc_is_tx;

void
__wrap_neighbor_discovery_open(struct neighbor_discovery_conn *c,
			     uint16_t channel,
			     clock_time_t initial,
			     clock_time_t min,
			     clock_time_t max,
			     const struct neighbor_discovery_callbacks *u) {
	mock().actualCall("neighbor_discovery_open");
	memcpy(&fake_ndc, u, sizeof(fake_ndc));
}

void
__wrap_neighbor_discovery_close(struct neighbor_discovery_conn *c) {
	mock().actualCall("neighbor_discovery_close");
}

void
__wrap_neighbor_discovery_set_val(struct neighbor_discovery_conn *c, uint16_t val) {
	mock().actualCall("neighbor_discovery_set_val")
		.withParameter("val", val);
}

void
__wrap_neighbor_discovery_start(struct neighbor_discovery_conn *c, uint16_t val) {
	mock().actualCall("neighbor_discovery_start")
		.withParameter("val", val);
}

void
__wrap_runicast_open(struct runicast_conn *c, uint16_t channel,
	      const struct unicast_callbacks *u) {
	mock().actualCall("unicast_open");
	memcpy(&fake_ucc, u, sizeof(fake_ucc));
}

void
__wrap_runicast_close(struct runicast_conn *c) {
	mock().actualCall("unicast_close");
}

int
__wrap_runicast_send(struct runicast_conn *c, const linkaddr_t *receiver) {
	mock().actualCall("unicast_send");
	return 0;
}

int
__wrap_runicast_is_transmitting(struct runicast_conn * c) {
	return fake_rc_is_tx;
}

#ifdef __cplusplus
}
#endif//__cplusplus


TEST_GROUP(brass_net) {
	struct brass_net net;
	uint8_t addr_counter;

	void
	setup() {
        mock().expectOneCall("neighbor_discovery_open");
        mock().expectOneCall("neighbor_discovery_start")
            .withParameter("val", uint8_t(-1));
        mock().expectOneCall("unicast_open");
		brass_net_open(&net, 0);
		fake_rc_is_tx = 0;
	}

	void
	teardown() {
        mock().expectOneCall("neighbor_discovery_close");
        mock().expectOneCall("unicast_close");
		brass_net_close(&net);

		mock().ignoreOtherCalls();
		mock().checkExpectations();
		mock().clear();
	}

    void GetAddress(linkaddr_t * addr) {
        addr->u8[0] = ++addr_counter;
        addr->u8[1] = 0;
    }
};

TEST(brass_net, should_open_neighbor_discovery) {
    BYTES_EQUAL(brass_net_cycles(&net), -1);
    BYTES_EQUAL(brass_net_hops(&net), -1);
    CHECK(linkaddr_cmp(brass_net_parent(&net), &linkaddr_null));
}

TEST(brass_net, should_start_cycle_after_neighbor_query) {
    fake_ndc.sent(&net.nd);
    BYTES_EQUAL(brass_net_cycles(&net), 0);
}

TEST(brass_net, should_discover_neighbor) {
    uint8_t hops = 3;
    linkaddr_t neighbor_addr;
    GetAddress(&neighbor_addr);

    mock().expectOneCall("neighbor_discovery_set_val")
        .withParameter("val", hops + 1);

    fake_ndc.recv(&net.nd, &neighbor_addr, hops);
    CHECK(linkaddr_cmp(brass_net_parent(&net), &neighbor_addr));
    BYTES_EQUAL(hops, brass_net_hops(&net));
}

TEST(brass_net, should_discover_near_neighbor) {
    uint8_t old_hops = 5;
    linkaddr_t old_parent_addr;
    GetAddress(&old_parent_addr);

    uint8_t hops = 4;
    linkaddr_t neighbor_addr;
    GetAddress(&neighbor_addr);

    mock().expectOneCall("neighbor_discovery_set_val")
        .withParameter("val", old_hops + 1);

    mock().expectOneCall("neighbor_discovery_set_val")
        .withParameter("val", hops + 1);

    brass_net_set_hops(&net, old_hops);
    brass_net_set_parent(&net, &old_parent_addr);

    fake_ndc.recv(&net.nd, &neighbor_addr, hops);
    CHECK(linkaddr_cmp(brass_net_parent(&net), &neighbor_addr));
    BYTES_EQUAL(hops, brass_net_hops(&net));
}

TEST(brass_net, should_discover_farther_neighbor) {
    uint8_t old_hops = 4;
    linkaddr_t old_parent_addr;
    GetAddress(&old_parent_addr);
    
    uint8_t hops = 5;
    linkaddr_t neighbor_addr;
    GetAddress(&neighbor_addr);

    mock().expectOneCall("neighbor_discovery_set_val")
        .withParameter("val", old_hops + 1);

    brass_net_set_hops(&net, old_hops);
    brass_net_set_parent(&net, &old_parent_addr);

    fake_ndc.recv(&net.nd, &neighbor_addr, hops);
    CHECK(linkaddr_cmp(brass_net_parent(&net), &old_parent_addr));
    BYTES_EQUAL(old_hops, brass_net_hops(&net));
}

TEST(brass_net, should_bind_an_app) {
	struct brass_app app;
	brass_app_init(&app);
	brass_net_bind(&net, &app);
	BYTES_EQUAL(1, brass_net_size(&net));
	POINTERS_EQUAL(&net, app.net);
}

TEST(brass_net, should_unbind_app) {
	struct  brass_app app;
	brass_app_init(&app);
	brass_net_bind(&net, &app);
	BYTES_EQUAL(1, brass_net_size(&net));

	brass_net_unbind(&net, &app);
	BYTES_EQUAL(0, brass_net_size(&net));
	POINTERS_EQUAL(NULL, app.net);
	brass_app_cleanup(&app);
}

TEST(brass_net, should_flush_gather_in_batch) {
	struct brass_app app[2];
	struct brass_pair * pair[2];

	app[0].id = 1;
	brass_app_init(&app[0]);
	brass_net_bind(&net, &app[0]);
	pair[0] = brass_pair_alloc(&app[0], 1, 1);
	pair[0]->key[0] = 5;
	pair[0]->value[0] = 10;
	brass_app_emit(&app[0], pair[0]);

	app[1].id = 2;
	brass_app_init(&app[1]);
	brass_net_bind(&net, &app[1]);
	pair[1] = brass_pair_alloc(&app[1], 1, 1);
	pair[1]->key[0] = 5;
	pair[1]->value[0] = 10;
	brass_app_emit(&app[1], pair[1]);
	
	mock().expectOneCall("unicast_send");
	
	linkaddr_t parent_addr = { { 1, 1 } };
	brass_net_set_parent(&net, &parent_addr);
	brass_net_flush(&net, 0);

	uint8_t buf[] = { 0, app[0].id, 6, 1, 1, 5, 10, app[1].id, 6, 1, 1, 5, 10 };
	CHECK(memcmp(packetbuf_dataptr(), buf, sizeof(buf)) == 0);
	
	BYTES_EQUAL(brass_app_size(&app[0]), 0);
	brass_app_cleanup(&app[0]);
	brass_pair_free(pair[0]);

	BYTES_EQUAL(brass_app_size(&app[1]), 0);
	brass_app_cleanup(&app[1]);
	brass_pair_free(pair[1]);
}

TEST(brass_net, should_not_flush_when_no_parent) {
	struct brass_app app;
	brass_app_init(&app);
	brass_net_bind(&net, &app);

	struct brass_pair * pair = brass_pair_alloc(&app, 1, 1);
	pair->key[0] = 5;
	pair->value[0] = 10;
	brass_app_emit(&app, pair);
	brass_pair_free(pair);

	brass_net_set_parent(&net, &linkaddr_null);
	CHECK(!brass_net_flush(&net, 0));

	BYTES_EQUAL(brass_app_size(&app), 1);
	brass_app_cleanup(&app);
}

TEST(brass_net, should_not_flush_when_is_transmitting) {
	struct brass_app app;
	brass_app_init(&app);
	brass_net_bind(&net, &app);

	struct brass_pair * pair = brass_pair_alloc(&app, 1, 1);
	pair->key[0] = 5;
	pair->value[0] = 10;
	brass_app_emit(&app, pair);
	brass_pair_free(pair);

	fake_rc_is_tx = 1;
	linkaddr_t parent = { { 1, 1 } };
	brass_net_set_parent(&net, &parent);
	CHECK(!brass_net_flush(&net, 0));

	BYTES_EQUAL(brass_app_size(&app), 1);
	brass_app_cleanup(&app);
}

TEST(brass_net, should_recv_feed_in_batch) {
	struct brass_app app[2];
	uint8_t key;
	
	app[0].id = 1;
	brass_app_init(&app[0]);
	brass_net_bind(&net, &app[0]);

	app[1].id = 2;
	brass_app_init(&app[1]);
	brass_net_bind(&net, &app[1]);

	linkaddr_t child_addr;
    GetAddress(&child_addr);
	
	uint8_t buffer[] = { 0, app[1].id, 10, 1, 1, 'C', 'D', 1, 1, 'A', 'B', app[0].id, 6, 1, 1, 'E', 'F', 0, 0, 0 };
    packetbuf_copyfrom(buffer, sizeof(buffer));
    fake_ucc.recv(&net.uc, &child_addr, 0);
	
	BYTES_EQUAL(brass_app_size(&app[0]), 1);
	key = 'E';
	BYTES_EQUAL(brass_app_find(&app[0], &key, sizeof(key))->value[0], 'F');
	CHECK(!brass_pair_urgent(brass_app_find(&app[0], &key, sizeof(key))));
	brass_app_cleanup(&app[0]);

	BYTES_EQUAL(brass_app_size(&app[1]), 2);
	key = 'A';
	BYTES_EQUAL(brass_app_find(&app[1], &key, sizeof(key))->value[0], 'B');
	key = 'C';
	BYTES_EQUAL(brass_app_find(&app[1], &key, sizeof(key))->value[0], 'D');
	brass_app_cleanup(&app[1]);
}

TEST(brass_net, should_recv_urgent) {
	struct brass_app app;

	app.id = 2;
	brass_app_init(&app);
	brass_net_bind(&net, &app);
	
	linkaddr_t parent_addr = { { 1, 1 } };
	brass_net_set_parent(&net, &parent_addr);

	linkaddr_t child_addr;
    GetAddress(&child_addr);
	
	mock().expectOneCall("unicast_send");
	
	uint8_t buffer[] = { 1, app.id, 6, 1, 1, 'C', 'D', 0, 0, 0 };
    packetbuf_copyfrom(buffer, sizeof(buffer));
    fake_ucc.recv(&net.uc, &child_addr, 0);
	
	brass_app_print(&app, "urgent-test ");
	BYTES_EQUAL(brass_app_size(&app), 0);
	brass_app_cleanup(&app);
}
