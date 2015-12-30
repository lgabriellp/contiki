#include <contiki.h>
#include <powertrace.h>
#include <dev/serial-line.h>
#include <brass.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_APPS 2
#define POWERTRACE_PERIOD	(60)

uint16_t node_loc_x = -1;
uint16_t node_loc_y = -1;
uint16_t node_loc_r = 0;

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
map(struct brass_app * app, int8_t type, int8_t value) {
	struct brass_pair * pair = brass_pair_alloc(app, sizeof(collect_key_t), sizeof(collect_value_t));
	collect_key_t ckey;
	collect_value_t cvalue;

	ckey.date = clock_seconds() / 36000;
	ckey.pos_x = node_loc_x;
	ckey.pos_y = node_loc_y;
	ckey.type = type;

	cvalue.sqr_sum = value * value;
	cvalue.sum = value;
	cvalue.count = 1;
	
	brass_pair_set_key(pair, &ckey);
	brass_pair_set_value(pair, &cvalue);

	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

static void
reduce(struct brass_app * app, struct brass_pair * acc, const int8_t * next) {
	collect_value_t * cacc = (collect_value_t *)acc->value;
	collect_value_t * cnext = (collect_value_t *)next;

	cacc->sqr_sum += cnext->sqr_sum;
	cacc->sum += cnext->sum;
	cacc->count += cnext->count;
}

PROCESS(brass_process, "Mixed Brass");
AUTOSTART_PROCESSES(&brass_process);

PROCESS_THREAD(brass_process, ev, data) {
	static struct brass_app app[NUM_APPS];
	static struct brass_net net;
	static char buffer[30];
	static int i;

	PROCESS_EXITHANDLER({
		brass_net_close(&net);
	});

	PROCESS_BEGIN();
	
	printf("input\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_x = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_y = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_r = atoi((char *)data) != 0;
	printf("#P %2.d %3.d %3.d %1.d %3.d %u\n", NUM_APPS, node_loc_x, node_loc_y, node_loc_r, POWERTRACE_PERIOD, RTIMER_SECOND);
	
	powertrace_start(POWERTRACE_PERIOD * CLOCK_SECOND);
	brass_net_open(&net, linkaddr_node_addr.u8[0] == 1);
	
	for (i = 0; i < NUM_APPS; i++) {
		app[i].id = i + 1;
		app[i].map = map;
		app[i].reduce = reduce;
		app[i].flush_period = 20 * 60;
		app[i].sow_period = 5 * 60;
		brass_net_bind(&net, &app[i]);
	}

	brass_net_sched_flush(&net);

	while (1) {
		PROCESS_YIELD_UNTIL(etimer_expired(&net.flush_timer));
		brass_net_sched_flush(&net);

		brass_net_flush(&net, 0);
		printf("flushed normal size=%d\n", brass_app_size(app, BRASS_FLAG_ALL));
	}
	
	PROCESS_END();
}
