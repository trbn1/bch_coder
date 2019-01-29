#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h>
#include "bch.h"
#include "const.h"
#include "timers.h"

void loop_print(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, uint16_t bench_iter, FILE *log);

void csv_print(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, uint16_t bench_iter, FILE *log);

void full_print(struct bch_enc *bch_enc, struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, FILE *log);

#endif
