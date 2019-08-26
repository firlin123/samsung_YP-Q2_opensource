#ifndef _TOUCHPAD_H_
#define _TOUCHPAD_H_

int touchpad_interval;
//float touchpad_interval;
//typedef struct {
//	int interval;
//} touchpad_interval;

#define TOUCHPAD_WRITE	_IOW('t', 0, int)

#endif
