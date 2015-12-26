#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <contiki.h>
#include <sys/etimer.h>
#include <sys/stimer.h>
#include <powertrace.h>
#include <dev/serial-line.h>
#include <lib/random.h>
#include <stdlib.h>
#include <stdio.h>
#include <collect-app.h>
#define NUM_APPS 5

#ifdef __cplusplus
}
#endif//__cplusplus


//#define COLLECT_FLUSH (60 * 60)
//#define COLLECT_SOW   (60 * 15)

#define COLLECT_FLUSH		(1 * (10 + (random_rand() % 20)) * CLOCK_SECOND)
#define COLLECT_SOW			(1 * CLOCK_SECOND)
#define POWERTRACE_PERIOD	(10 * CLOCK_SECOND)

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

void
collect_map(struct brass_app * app, int8_t type, int8_t value) {
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(collect_key_t), sizeof(collect_value_t));
	collect_key_t ckey;
	collect_value_t cvalue;

	ckey.date = clock_seconds() / 3600;
	ckey.pos_x = node_loc_x;
	ckey.pos_y = node_loc_y;
	ckey.type = type;

	cvalue.sqr_sum = value * value;
	cvalue.sum = value;
	cvalue.count = 1;
	
	brass_pair_set_key(pair, &ckey);
	brass_pair_set_value(pair, &cvalue);

	brass_pair_print(pair, "emited  ");
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

void
collect_reduce(struct brass_app * app, struct brass_pair * acc, const int8_t * next) {
	collect_value_t * cacc = (collect_value_t *)acc->value;
	collect_value_t * cnext = (collect_value_t *)next;

	cacc->sqr_sum += cnext->sqr_sum;
	cacc->sum += cnext->sum;
	cacc->count += cnext->count;
}

uint8_t node_loc_x = -1;
uint8_t node_loc_y = -1;

PROCESS(collect_process, "Collect App");
AUTOSTART_PROCESSES(&collect_process);

PROCESS_THREAD(collect_process, ev, data) {
	static struct etimer sow_timer;
	static struct etimer flush_timer;

	static struct brass_net net;
	static struct brass_app app[NUM_APPS];
	static int i;
	static clock_time_t flush_period;

	PROCESS_EXITHANDLER({
		for (i = 0; i < NUM_APPS; i++) {
			brass_app_cleanup(&app[i], BRASS_FLAG_ALL);
		}
		brass_net_close(&net);   
	});
	
	PROCESS_BEGIN();

	printf("waiting position\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_x = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_y = atoi((char *)data);
	printf("pos (%d, %d)\n", node_loc_x, node_loc_y);
	printf("num_apps=%d\n", NUM_APPS);

	powertrace_start(60 * CLOCK_SECOND);
	brass_net_open(&net, linkaddr_node_addr.u8[0] == 1);

	for (i = 0; i < NUM_APPS; i++) {
		app[i].id = i+1;
		app[i].map = collect_map;
		app[i].reduce = collect_reduce;
		brass_app_init(&app[i]);
		brass_net_bind(&net, &app[i]);
	}
	
	etimer_set(&sow_timer, COLLECT_SOW);
	flush_period = COLLECT_FLUSH;
	printf("sched timer %lu s\n", flush_period / CLOCK_SECOND);
	etimer_set(&flush_timer, flush_period);

	while(1) {
		PROCESS_YIELD_UNTIL(etimer_expired(&sow_timer) || etimer_expired(&flush_timer));

		if (linkaddr_node_addr.u8[0] == 1) {
			for (i = 0; i < NUM_APPS; i++) {
				brass_app_print(&app[i], "result-1 ");
			}
			continue;
		}

		if (etimer_expired(&sow_timer)) {
			etimer_set(&sow_timer, COLLECT_SOW);
			for (i = 0; i < NUM_APPS; i++) {
				brass_app_sow(&app[i], BRASS_SENSOR_TEMP, 1);
			}
			brass_net_flush(&net, 1);
		}

		if (etimer_expired(&flush_timer)) {
			flush_period = COLLECT_FLUSH;
			printf("sched timer %lu s\n", flush_period / CLOCK_SECOND);
			etimer_set(&flush_timer, flush_period);
			brass_net_flush(&net, 0);
		}
	}

	PROCESS_END();
}
