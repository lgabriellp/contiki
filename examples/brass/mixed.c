#include <contiki.h>
#include <brass.h>

uint16_t node_loc_x = -1;
uint16_t node_loc_y = -1;

PROCESS(brass_process, "Brass Process");
AUTOSTART_PROCESSES(&brass_process);

PROCESS_THREAD(brass_process, ev, data) {
	static struct brass_net net;
	static struct brass_app app;

	PROCESS_EXITHANDLER({
		brass_net_close(&net);   
	});
	
	PROCESS_BEGIN();

	printf("waiting position\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_x = atoi((char *)data);
	PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
	node_loc_y = atoi((char *)data);
	printf("pos (%d, %d)\n", node_loc_x, node_loc_y);

	powertrace_start(POWERTRACE_PERIOD);
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
