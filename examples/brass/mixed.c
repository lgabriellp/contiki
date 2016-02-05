#include <contiki.h>
#include <powertrace.h>
#include <dev/serial-line.h>
#include <lib/random.h>
#include <brass.h>
#include <stdio.h>
#include <stdlib.h>

#define POWERTRACE_PERIOD	(60)
#define COMMON_TIME_RANGE	(60 * 30)
#define MIXES_RATIO_EXPERIMENT
/*
#define NUM_APPS 1
#define COLLECT_FLUSH		(60 * 20)
#define COLLECT_SOW			(60 * 5)
#define DETECT_FLUSH		(60 * 20)
#define DETECT_SOW			(10)
*/

uint8_t APPS_SIZE;
uint8_t APPS_TYPE;
uint8_t NODE_LOC_X = -1;
uint8_t NODE_LOC_Y = -1;
uint8_t NODE_LOC_R = 0;
uint8_t NODE_LOC_F = 0;

clock_time_t COLLECT_SOW;
clock_time_t COLLECT_FLUSH;
clock_time_t DETECT_SOW;
clock_time_t DETECT_FLUSH;

typedef struct __attribute__ ((packed)) collect_key {
	unsigned date:16;
	unsigned pos_x:8;
	unsigned pos_y:8;
	unsigned type:8;
} collect_key_t;

typedef struct __attribute__ ((packed)) collect_value {
	unsigned sqr_sum:16;
	unsigned sum:8;
	unsigned count:8;
} collect_value_t;

static void
collect_normal_map(struct brass_app * app, int8_t type, int8_t value) {
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(collect_key_t), sizeof(collect_value_t));
	collect_key_t ckey;
	collect_value_t cvalue;
	
	ckey.pos_x = 0;
	ckey.pos_y = 0;
	ckey.date = clock_seconds() / COMMON_TIME_RANGE;
	ckey.type = 1;

	cvalue.sqr_sum = value * value;
	cvalue.sum = value;
	cvalue.count = 1;
	
	brass_pair_set_value(pair, &cvalue);
	brass_pair_set_key(pair, &ckey);
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

static void
collect_fire_map(struct brass_app * app, int8_t type, int8_t sample) {
	if (!NODE_LOC_F) return;

	printf("fire!\n");
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(collect_key_t), sizeof(collect_value_t));
	collect_key_t key;
	collect_value_t value;

	key.date = clock_seconds() / 60;
	key.pos_x = NODE_LOC_X;
	key.pos_y = NODE_LOC_Y;
	key.type = 1;

	value.sqr_sum = sample * sample;
	value.sum = sample;
	value.count = 1;
	
	brass_pair_set_value(pair, &value);
	brass_pair_set_key(pair, &key);
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

static void
collect_reduce(struct brass_app * app, struct brass_pair * result, const int8_t * value) {
	collect_value_t * result_value = (collect_value_t *)result->value;
	collect_value_t * next_value = (collect_value_t *)value;

	result_value->sqr_sum += next_value->sqr_sum;
	result_value->sum += next_value->sum;
	result_value->count += next_value->count;
}

typedef struct __attribute__((packed)) {
	unsigned date:16;
	unsigned type:8;
	unsigned pos_x:8;
	unsigned pos_y:8;
	unsigned animal_id:8;
} detect_key_t;

void
detect_normal_map(struct brass_app * app, int8_t type, int8_t value) {
#ifndef MIXES_RATIO_EXPERIMENT
#warn MIXES_RATIO_EXPERIMENT
	if (random_rand() % 360) return;
#endif//!MIXES_RATIO_EXPERIMENT
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(detect_key_t), 0);
	detect_key_t key;
	
	key.date = clock_seconds() / COMMON_TIME_RANGE;
	key.type = 2;
	key.pos_x = NODE_LOC_X;
	key.pos_y = NODE_LOC_Y;
	key.animal_id = value;

	brass_pair_set_key(pair, &key);
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

void
detect_road_map(struct brass_app * app, int8_t type, int8_t value) {
	if (!NODE_LOC_R) return;
#ifndef MIXES_RATIO_EXPERIMENT
#warn MIXES_RATIO_EXPERIMENT
	if (random_rand() % 3600) return;
#endif//!MIXES_RATIO_EXPERIMENT

	printf("road!\n");

	struct brass_pair * pair = brass_pair_alloc(app, sizeof(detect_key_t), 0);
	brass_pair_set_flags(pair, BRASS_FLAG_URGENT, 1);
	detect_key_t key;
	
	key.date = clock_seconds() / 60;
	key.type = 2;
	key.pos_x = NODE_LOC_X;
	key.pos_y = NODE_LOC_Y;
	key.animal_id = value;

	brass_pair_set_key(pair, &key);
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

void
detect_reduce(struct brass_app * app, struct brass_pair * acc, const int8_t * next) {

}

PROCESS(brass_process, "Mixed Brass");
AUTOSTART_PROCESSES(&brass_process);

PROCESS_THREAD(brass_process, ev, data) {
	static struct brass_app * app;
	static struct brass_net net;
	
	static struct brass_app collect_normal;
	static struct brass_app collect_fire;
	static struct brass_app detect_normal;
	static struct brass_app detect_road;

	static int i;

	PROCESS_EXITHANDLER({
		brass_net_close(&net);
	});

	PROCESS_BEGIN();

	printf("input\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	APPS_SIZE = atoi((char *)data);

	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	APPS_TYPE = atoi((char *)data);

	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	NODE_LOC_X = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	NODE_LOC_Y = atoi((char *)data);

	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	NODE_LOC_R = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	NODE_LOC_F = atoi((char *)data);

	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	COLLECT_SOW = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	COLLECT_FLUSH = atoi((char *)data);

	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	DETECT_SOW = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	DETECT_FLUSH = atoi((char *)data);

	printf("CONTEXT");
   	printf(" %" PRIu32, clock_seconds());
	printf(" %" PRIu8, linkaddr_node_addr.u8[0]);
	printf(" %u %u", APPS_SIZE, APPS_TYPE);
	printf(" %u %u", NODE_LOC_X, NODE_LOC_Y);
	printf(" %u %u", NODE_LOC_R, NODE_LOC_F);
	printf(" %u %u", (unsigned)COLLECT_SOW, (unsigned)COLLECT_FLUSH);
	printf(" %u %u", (unsigned)DETECT_SOW, (unsigned)DETECT_FLUSH);
	printf(" %u %u", POWERTRACE_PERIOD, RTIMER_SECOND);
	printf("\n");

	collect_normal.map = collect_normal_map;
	collect_normal.reduce = collect_reduce;
	collect_normal.sow_period = COLLECT_SOW;
	collect_normal.flush_period = COLLECT_FLUSH;

	memcpy(&collect_fire, &collect_normal, sizeof(struct brass_app));
	collect_fire.map = collect_fire_map;

	detect_normal.map = detect_normal_map;
	detect_normal.reduce = detect_reduce;
	detect_normal.sow_period = DETECT_SOW;
	detect_normal.flush_period = DETECT_FLUSH;

	memcpy(&detect_road, &detect_normal, sizeof(struct brass_app));
	detect_road.map = detect_road_map;

	app = (struct brass_app *)malloc(APPS_SIZE * sizeof(struct brass_app));

	powertrace_start(POWERTRACE_PERIOD * CLOCK_SECOND);
	brass_net_open(&net, linkaddr_node_addr.u8[0] == 1);
	
	struct brass_app * app_type;
	int app_index = 0;

	for (i = 0; i < APPS_SIZE; i++) {
		app_index = APPS_TYPE == 0 ? i % 4 : (APPS_TYPE - 1);
		printf("app_index=%d\n", app_index);

		switch (app_index) {
		case 0: app_type = &collect_normal; break;
		case 1: app_type = &collect_fire; break;
		case 2: app_type = &detect_normal; break;
		case 3: app_type = &detect_road; break;
		};
		memcpy(&app[i], app_type, sizeof(struct brass_app));
		app[i].id = i + 1;
		brass_net_bind(&net, &app[i]);
	}

	brass_net_sched_flush(&net);

	while (1) {
		PROCESS_YIELD_UNTIL(etimer_expired(&net.flush_timer));
		brass_net_sched_flush(&net);

		brass_net_flush(&net, 0);
	}
	
	PROCESS_END();
}
