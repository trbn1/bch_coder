#ifndef CONST_H
#define CONST_H

#define CODE_LENGTH 0x1ff
#define DATA_LENGTH 0xdc
#define ERR_CORR_CAPABILITY 0x27
#define CONTROL_LENGTH 0x124

#define ERR_AMOUNT 0x3
#define LOOPS 0x1

struct user_settings {
	const uint8_t err_type;
	const uint8_t err_amount;

	const uint8_t benchmark;
	const uint16_t bench_loops;

	const uint8_t csv;
	const uint8_t global_highlight;
	const uint8_t log_trim;
};

#endif