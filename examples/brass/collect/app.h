#ifndef _COLLECT_H_
#define _COLLECT_H_
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <brass.h>

extern uint8_t node_loc_x;
extern uint8_t node_loc_y;

void collect_map(struct brass_app * app, int8_t type, int8_t value);
void collect_reduce(struct brass_app * app, struct brass_pair * acc, const int8_t * next);

#ifdef __cplusplus
}
#endif//__cplusplus
#endif//_COLLECT_H_
