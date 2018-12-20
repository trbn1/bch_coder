#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <time.h>
#include "const.h"

struct timers {
	uint64_t exec_start;
	uint64_t exec_stop;
	uint64_t enc_start;
	uint64_t enc_stop;
	uint64_t dec_start;
	uint64_t dec_stop;

	double exec_times[LOOPS];
	double enc_times[LOOPS];
	double dec_times[LOOPS];
};

uint64_t get_time(struct timespec ts);

void reset_timers(struct timers *timers);

#endif