#ifndef _DETECT_H_
#define _DETECT_H_
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <brass.h>

extern uint8_t node_loc_x;
extern uint8_t node_loc_y;

#define PRESENCE_DETECTED_EVENT 1
void detect_map(struct brass_app * app, int8_t type, int8_t value);
void detect_reduce(struct brass_pair * acc, const int8_t * next);

#ifdef __cplusplus
}
#endif//__cplusplus
#endif//_DETECT_H_
