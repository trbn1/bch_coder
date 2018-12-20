#include "headers/bch.h"
#include "headers/const.h"
#include "headers/log.h"
#include "headers/timers.h"

int main(void)
{
	/* userset */
	struct user_settings us = {
		.err_type = 0x0,
		.err_amount = ERR_AMOUNT,

		.benchmark = 0x0,
		.bench_loops = LOOPS,

		.csv = 0x0,
		.global_highlight = 0x0,
		.log_trim = 0x32
	};

	/* init */
	const uint16_t gen_poly_tab[] = {
		0x0, 0x2, 0x3, 0x6, 0x7, 0x8, 0xa, 0xb,
		0xc, 0xe, 0x10, 0x11, 0x12, 0x13, 0x18, 0x1b,
		0x1f, 0x20, 0x21, 0x22, 0x24, 0x28, 0x29, 0x2b,
		0x2d, 0x2e, 0x2f, 0x30, 0x35, 0x36, 0x37, 0x38,
		0x3c, 0x3e, 0x3f, 0x41, 0x42, 0x45, 0x46, 0x48,
		0x4a, 0x4d, 0x4e, 0x4f, 0x51, 0x53, 0x57, 0x5a,
		0x5e, 0x60, 0x61, 0x63, 0x65, 0x67, 0x68, 0x6c,
		0x6d, 0x6e, 0x71, 0x77, 0x78, 0x79, 0x7a, 0x7b,
		0x7c, 0x7d, 0x80, 0x81, 0x84, 0x85, 0x86, 0x88,
		0x8b, 0x8d, 0x8e, 0x93, 0x94, 0x95, 0x96, 0x99,
		0x9a, 0x9b, 0x9e, 0x9f, 0xa0, 0xa4, 0xa5, 0xa6,
		0xa9, 0xac, 0xad, 0xb5, 0xb6, 0xb7, 0xbb, 0xbd,
		0xbe, 0xc1, 0xc3, 0xc6, 0xc7, 0xc8, 0xca, 0xcb,
		0xd3, 0xd5, 0xd7, 0xd9, 0xda, 0xdc, 0xdd, 0xde,
		0xe1, 0xe3, 0xe6, 0xe8, 0xea, 0xeb, 0xed, 0xee,
		0xf0, 0xf1, 0xf2, 0xf4, 0xf5, 0xf6, 0xf8, 0xfb,
		0xfc, 0xfd, 0xfe, 0x100, 0x102, 0x105, 0x106, 0x10b,
		0x10c, 0x10d, 0x10e, 0x10f, 0x112, 0x113, 0x114, 0x115,
		0x116, 0x117, 0x118, 0x119, 0x11b, 0x11c, 0x11d, 0x120,
		0x121, 0x122, 0x123,
	};
	const size_t gen_poly_tab_size = sizeof(gen_poly_tab) / sizeof(gen_poly_tab[0]);

	struct timers timers = { 0 };
	struct bch_enc bch_enc = { 0 };
	struct bch_sdec bch_sdec = { 0 };
	struct timespec ts = { 0 };

	srand((uint16_t)time(NULL));
	/* end init */

	/* Initialize generator */
	init_generator(gen_poly_tab, gen_poly_tab_size, &bch_enc, &bch_sdec);

	FILE *log;
	errno_t err;
	if ((err = fopen_s(&log, "out.log", "w")) != 0) {
		perror("Error while opening file");
		system("PAUSE");
		return 1;
	}
	else {
		uint16_t bench_iter = 0;
		do {
			if (bench_iter > 0) {
				/* Reset variables */
				reset_timers(&timers);
				reset_bch_enc(&bch_enc);
				reset_bch_sdec(&bch_sdec);
			}

			timers.exec_start = get_time(ts);

			/* Generating random data */
			generate_random_data(&bch_enc);

			/* Encoding data */
			timers.enc_start = get_time(ts);

			encode_data(&bch_enc);

			timers.enc_stop = get_time(ts);

			/* Receive encoded data */
			receive_encoded_data(&bch_enc, &bch_sdec);

			/* Add errors */
			if (us.err_amount > 0) {
				add_errors(&bch_sdec, &us);
			}

			/* Decoding data (simple)*/
			memcpy(bch_sdec.shifted_recv_vector, bch_sdec.recv_vector, sizeof(bch_sdec.shifted_recv_vector));

			timers.dec_start = get_time(ts);

			sdecode_data(&bch_sdec);

			timers.dec_stop = get_time(ts);

			timers.exec_stop = get_time(ts);

			timers.enc_times[bench_iter] = (double)(timers.enc_stop - timers.enc_start) / 0xf4240;
			timers.dec_times[bench_iter] = (double)(timers.dec_stop - timers.dec_start) / 0xf4240;
			timers.exec_times[bench_iter] = (double)(timers.exec_stop - timers.exec_start) / 0xf4240;

			if (us.benchmark == 1 && us.csv == 0) {
				loop_print(&bch_sdec, &timers, &us, bench_iter, log);
			}
			if (us.csv == 1) {
				csv_print(&bch_sdec, &timers, &us, bench_iter, log);
			}
			bench_iter++;
		} while (bench_iter < us.bench_loops);

		if (us.benchmark == 0 && us.bench_loops == 1) {
			full_print(&bch_enc, &bch_sdec, &timers, &us, log);
		}
	}
}
