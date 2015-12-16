#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <contiki.h>
#include <dev/serial-line.h>
#include <stdlib.h>
#include <stdio.h>
#include <detect/app.h>

#ifdef __cplusplus
}
#endif//__cplusplus


typedef struct __attribute__((packed)) {
	unsigned date:16;
	unsigned animal_id:8;
} detect_key_t;

void
detect_map(struct brass_app * app, int8_t type, int8_t value) {
	if (!value) return;

	struct brass_pair * pair = brass_pair_alloc(app, sizeof(detect_key_t), 0);
	brass_pair_set_flags(pair, BRASS_FLAG_URGENT);
	detect_key_t key;

	key.animal_id = value;
	key.date = clock_seconds();
	memcpy(pair->key, &key, sizeof(key));
	brass_app_emit(app, pair);
	brass_pair_free(pair);
}

void
detect_reduce(struct brass_app * app, struct brass_pair * acc, const int8_t * next) {

}

PROCESS(detect_process, "Detect App");
AUTOSTART_PROCESSES(&detect_process);

PROCESS_THREAD(detect_process, ev, data) {
	static struct etimer timer;
	static struct brass_net net;
	static struct brass_app app;
	static int round = 0;

	PROCESS_EXITHANDLER({
		brass_app_cleanup(&app, BRASS_FLAG_ALL);
		brass_net_close(&net);   
	});

	PROCESS_BEGIN();
	
	app.map = detect_map;
	app.reduce = detect_reduce;
	app.id = 1;

	printf("waiting position\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_x = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_y = atoi((char *)data);
	printf("reply (%d, %d)\n", node_loc_x, node_loc_y);

	brass_app_init(&app);
	brass_net_open(&net, linkaddr_node_addr.u8[0] == 1);
	brass_net_bind(&net, &app);

	while(1) {
		etimer_set(&timer, CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		if (linkaddr_node_addr.u8[0] != 1) {
			brass_app_sow(&app, BRASS_SENSOR_TEMP, 1);
			if (round++ % 10) continue;

			printf("flushing\n");
			brass_net_flush(&net, 0);
		}	
	}

	PROCESS_END();
}
