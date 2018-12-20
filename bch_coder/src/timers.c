#include "headers/timers.h"

uint64_t get_time(struct timespec ts) {
	timespec_get(&ts, TIME_UTC);
	return ts.tv_sec * 0x3b9aca00 + ts.tv_nsec;
}

void reset_timers(struct timers *timers) {
	timers->exec_start;
	timers->exec_stop;
	timers->enc_start;
	timers->enc_stop;
	timers->dec_start;
	timers->dec_stop;
}