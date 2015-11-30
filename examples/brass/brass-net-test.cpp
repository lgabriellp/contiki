#include <brass.h>
#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct neighbor_discovery_callbacks ndc;
struct unicast_callbacks ucc;

void __wrap_neighbor_discovery_open(struct neighbor_discovery_conn *c,
			     uint16_t channel,
			     clock_time_t initial,
			     clock_time_t min,
			     clock_time_t max,
			     const struct neighbor_discovery_callbacks *u) {
	mock().actualCall("neighbor_discovery_open");
	memcpy(&ndc, u, sizeof(ndc));
}

void __wrap_neighbor_discovery_close(struct neighbor_discovery_conn *c) {
	mock().actualCall("neighbor_discovery_close");
}

void __wrap_neighbor_discovery_set_val(struct neighbor_discovery_conn *c, uint16_t val) {
	mock().actualCall("neighbor_discovery_set_val")
		.withParameter("val", val);
}

void __wrap_neighbor_discovery_start(struct neighbor_discovery_conn *c, uint16_t val) {
	mock().actualCall("neighbor_discovery_start")
		.withParameter("val", val);
}

void __wrap_unicast_open(struct unicast_conn *c, uint16_t channel,
	      const struct unicast_callbacks *u) {
	mock().actualCall("unicast_open");
	memcpy(&ucc, u, sizeof(ucc));
}

void __wrap_unicast_close(struct unicast_conn *c) {
	mock().actualCall("unicast_close");
}

int __wrap_unicast_send(struct unicast_conn *c, const linkaddr_t *receiver) {
	mock().actualCall("unicast_send");
	return 0;
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
    ndc.sent(&net.nd);
    BYTES_EQUAL(brass_net_cycles(&net), 0);
}

TEST(brass_net, should_discover_neighbor) {
    uint8_t hops = 3;
    linkaddr_t neighbor_addr;
    GetAddress(&neighbor_addr);

    mock().expectOneCall("neighbor_discovery_set_val")
        .withParameter("val", hops + 1);

    ndc.recv(&net.nd, &neighbor_addr, hops);
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

    ndc.recv(&net.nd, &neighbor_addr, hops);
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

    ndc.recv(&net.nd, &neighbor_addr, hops);
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
}

TEST(brass_net, should_feed) {
	struct brass_app app;
	brass_app_init(&app);
	brass_net_bind(&net, &app);

    linkaddr_t children_addr;
    GetAddress(&children_addr);
	
	uint8_t buffer[] = { app.id, 2, 1, 1, 'C', 'D', 1, 1, 'A', 'B', 0, 0, 0 };
    packetbuf_copyfrom(buffer, sizeof(buffer));
    ucc.recv(&net.uc, &children_addr);

	BYTES_EQUAL(brass_app_size(&app), 2);
}

/*
TEST(brass_net, should_push_to_parent_reduced_data) {
    int16_t payload = 123;
    struct brass_app app;

    app.id = 10;
	brass_app_init(&app);

    brass_net_register_app(&net, &app);

    mock().expectOneCall("unicast_send")
        .withParameter("to", brass_net_parent(&net));

    brass_net_push(&net);

    struct packet {
        uint8_t id;
        uint8_t size;
        uint16_t payload;
    } packet;

    BYTES_EQUAL(packetbuf_datalen(), sizeof(packet));
    memcpy(&packet, packetbuf_dataptr(), sizeof(packet));
    BYTES_EQUAL(app.id, packet.id);
    BYTES_EQUAL(app.size, packet.size);
    BYTES_EQUAL(payload, packet.payload);
}

IGNORE_TEST(brass_net, should_notify_children_pushed_data) {
    struct payload2 {
        uint8_t cycle;
        uint8_t app;
        uint16_t key;
        uint16_t value;
    } payload;

    payload.cycle = 1;
    payload.app = 2;
    payload.key = 10;
    payload.value = 20;

    packetbuf_copyfrom(&payload, sizeof(payload));

    mock().expectOneCall("mapreduce_emit")
        .withParameter("key", payload.key);

    linkaddr_t children_addr;
    GetAddress(&children_addr);
    fake_ucc.recv(&net.uc, &children_addr);
}
*/
