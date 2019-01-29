#ifndef CONST_H
#define CONST_H

#define FIELD_DEGREE 0x9
#define CODE_LENGTH 0x1ff
#define DATA_LENGTH 0xdc
#define ERR_CORR_CAPABILITY 0x27
#define CONTROL_LENGTH 0x124

#define ERR_AMOUNT 0x28
#define LOOPS 0x3e8

struct user_settings {
	const uint8_t err_type;
	uint8_t err_amount;

	const uint8_t benchmark;
	const uint16_t bench_loops;

	const uint8_t csv;
	const uint8_t global_highlight;
	const uint8_t log_trim;
};

#endif