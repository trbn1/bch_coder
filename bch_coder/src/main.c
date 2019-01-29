#include "headers/bch.h"
#include "headers/const.h"
#include "headers/log.h"
#include "headers/timers.h"

int main(void)
{
	/* userset */
	struct user_settings us = {
		.err_type = 0x1,
		.err_amount = 0x0,

		.benchmark = 0x1,
		.bench_loops = LOOPS,

		.csv = 0x1,
		.global_highlight = 0x0,
		.log_trim = 0x32
	};

	/* init */
	/* http://www.partow.net/programming/polynomials/index.html#deg09 */
	const uint8_t prim_poly_tab[] = {
		0x0, 0x4, 0x9,
	};
	const size_t prim_poly_tab_size = sizeof(prim_poly_tab) / sizeof(prim_poly_tab[0]);

	/* from generator.py */
	const uint16_t gen_poly_tab[] = {
		0x0, 0x2, 0x3, 0x5, 0x9, 0xb, 0xd, 0xe,
		0x10, 0x11, 0x13, 0x17, 0x18, 0x19, 0x1d, 0x20,
		0x22, 0x23, 0x26, 0x27, 0x29, 0x2c, 0x31, 0x32,
		0x33, 0x34, 0x35, 0x39, 0x3a, 0x40, 0x41, 0x42,
		0x43, 0x47, 0x49, 0x4a, 0x50, 0x51, 0x52, 0x53,
		0x54, 0x55, 0x56, 0x5d, 0x5e, 0x64, 0x66, 0x68,
		0x69, 0x6a, 0x6b, 0x6d, 0x73, 0x74, 0x76, 0x79,
		0x7a, 0x7c, 0x7e, 0x7f, 0x85, 0x86, 0x87, 0x88,
		0x89, 0x8b, 0x8e, 0x90, 0x91, 0x93, 0x94, 0x96,
		0x97, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa1,
		0xa2, 0xa5, 0xa6, 0xaa, 0xab, 0xb2, 0xb4, 0xb5,
		0xb6, 0xba, 0xbb, 0xbc, 0xbd, 0xbf, 0xc0, 0xc1,
		0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcd,
		0xce, 0xd0, 0xd2, 0xd7, 0xd8, 0xda, 0xdb, 0xde,
		0xdf, 0xe0, 0xe1, 0xe2, 0xe6, 0xec, 0xed, 0xee,
		0xef, 0xf0, 0xf1, 0xf3, 0xf7, 0xf8, 0xfb, 0xfd,
		0xff, 0x104, 0x10b, 0x10d, 0x10f, 0x110, 0x114, 0x116,
		0x117, 0x118, 0x11d, 0x11e, 0x120, 0x121, 0x123
	};
	const size_t gen_poly_tab_size = sizeof(gen_poly_tab) / sizeof(gen_poly_tab[0]);

	struct timers timers = { 0 };
	struct field_elements field_elements = { 0 };
	struct bch_enc bch_enc = { 0 };
	struct bch_sdec bch_sdec = { 0 };
	struct bch_dec bch_dec = { 0 };
	struct timespec ts = { 0 };

	srand((uint16_t)time(NULL));
	/* end init */

	/* Initialize field elements and generator */
	generate_field(prim_poly_tab, prim_poly_tab_size, &field_elements);
	init_generator(gen_poly_tab, gen_poly_tab_size, &bch_enc, &bch_sdec);

	for (us.err_amount = 0; us.err_amount <= ERR_AMOUNT; us.err_amount++) {
		FILE *log;
		errno_t err;
		char filename[256];
		snprintf(filename, sizeof filename, "%s%d%s", "out", us.err_amount, ".log");
		if ((err = fopen_s(&log, filename, "w")) != 0) {
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
					reset_bch_dec(&bch_dec);
				}

				timers.exec_start = get_time(ts);

				/* Generating random data */
				generate_random_data(&bch_enc);

				/* Encoding data */
				timers.enc_start = get_time(ts);

				encode_data(&bch_enc);

				timers.enc_stop = get_time(ts);

				/* Receive encoded data */
				receive_encoded_data(&bch_enc, &bch_sdec, &bch_dec);

				/* Add errors */
				if (us.err_amount > 0) {
					add_errors(&bch_sdec, &bch_dec, &us);
				}

				/* Decoding data (simple)*/
				memcpy(bch_sdec.shifted_recv_vector, bch_sdec.recv_vector, sizeof(bch_sdec.shifted_recv_vector));

				timers.sdec_start = get_time(ts);

				sdecode_data(&bch_sdec);

				timers.sdec_stop = get_time(ts);

				/* Decoding data */
				timers.dec_start = get_time(ts);

				decode_data(&field_elements, &bch_dec);

				timers.dec_stop = get_time(ts);

				timers.exec_stop = get_time(ts);

				timers.enc_times[bench_iter] = (double)(timers.enc_stop - timers.enc_start) / 0xf4240;
				timers.sdec_times[bench_iter] = (double)(timers.sdec_stop - timers.sdec_start) / 0xf4240;
				timers.dec_times[bench_iter] = (double)(timers.dec_stop - timers.dec_start) / 0xf4240;
				timers.exec_times[bench_iter] = (double)(timers.exec_stop - timers.exec_start) / 0xf4240;

				if (us.benchmark == 1 && us.csv == 0) {
					loop_print(&bch_sdec, &bch_dec, &timers, &us, bench_iter, log);
				}
				if (us.csv == 1) {
					csv_print(&bch_sdec, &bch_dec, &timers, &us, bench_iter, log);
				}
				bench_iter++;
			} while (bench_iter < us.bench_loops);

			if (us.benchmark == 0 && us.bench_loops == 1) {
				full_print(&bch_enc, &bch_sdec, &bch_dec, &timers, &us, log);
			}
		}
	}
}
