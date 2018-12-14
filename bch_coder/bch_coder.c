#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CODE_LENGTH 0x1ff
#define DATA_LENGTH 0xdc
#define ERR_CORR_CAPABILITY 0x27
#define CONTROL_LENGTH 0x124
#define LOOPS 0x1

int main(void)
{
	/* userset */
	static const uint8_t benchmark = 0x1;
	static const uint16_t bench_loops = LOOPS;
	static const uint8_t csv = 0x0;
	static const uint8_t err_type = 0x0;
	static const uint8_t global_highlight = 0x0;
	static const uint8_t err_amount = 0x4;
	static const uint8_t log_trim = 0x32;

	/* init */
	struct timespec ts;
	srand((uint16_t)time(NULL));

	uint32_t exec_start = 0;
	uint32_t exec_stop = 0;
	uint32_t enc_start = 0;
	uint32_t enc_stop = 0;
	uint32_t dec_start = 0;
	uint32_t dec_stop = 0;

	static const uint16_t gen_poly_tab[] = {
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
	static const size_t gen_poly_tab_size = sizeof(gen_poly_tab) / sizeof(gen_poly_tab[0]);
	uint16_t gen_poly_vector[CONTROL_LENGTH] = { 0 };

	uint8_t data_vector[DATA_LENGTH] = { 0 };
	uint16_t rest_vector[CONTROL_LENGTH] = { 0 };
	uint16_t result = 0;
	uint16_t enc_vector[CODE_LENGTH] = { 0 };

	uint16_t recv_vector[CODE_LENGTH] = { 0 };
	uint16_t recv_rest_vector[CONTROL_LENGTH] = { 0 };
	uint16_t shift = 0;
	uint8_t err_detected = 0;
	uint8_t dec_err = 0;

	uint16_t rand_err_location[ERR_CORR_CAPABILITY] = { 0 };

	uint16_t shifted_recv_vector[CODE_LENGTH] = { 0 };
	uint16_t weight = 0;

	uint16_t repaired_recv_vector[CODE_LENGTH] = { 0 };
	uint16_t err_loc[ERR_CORR_CAPABILITY] = { 0 };

	double exec_times[LOOPS] = { 0 };
	double enc_times[LOOPS] = { 0 };
	double dec_times[LOOPS] = { 0 };
	/* end init */

	/* Initializating generator and parameters */
	for (uint8_t i = 0; i < gen_poly_tab_size; i++)
		gen_poly_vector[gen_poly_tab[i]] = 1;

	FILE *log;
	errno_t err;
	if ((err = fopen_s(&log, "out.log", "w")) != 0) {
		return 1;
	}
	else {
		uint16_t b = 0;
		do {
			/* Reset variables */
			if (b > 0) {
				memset(data_vector, 0, sizeof(data_vector));
				memset(rest_vector, 0, sizeof(rest_vector));
				memset(enc_vector, 0, sizeof(enc_vector));
				memset(recv_vector, 0, sizeof(recv_vector));
				memset(recv_rest_vector, 0, sizeof(recv_rest_vector));
				memset(rand_err_location, 0, sizeof(rand_err_location));
				memset(shifted_recv_vector, 0, sizeof(shifted_recv_vector));
				memset(repaired_recv_vector, 0, sizeof(repaired_recv_vector));
				memset(err_loc, 0, sizeof(err_loc));
				result = 0;
				shift = 0;
				err_detected = 0;
				dec_err = 0;
				weight = 0;
			}

			timespec_get(&ts, TIME_UTC);
			exec_start = ts.tv_nsec;
			/* Generating random data */
			for (uint8_t i = 0; i < DATA_LENGTH; i++)
				data_vector[i] = rand() % 2;

			/* Encoding data */
			timespec_get(&ts, TIME_UTC);
			enc_start = ts.tv_nsec;

			for (int16_t i = DATA_LENGTH - 1; i >= 0; i--) {
				result = data_vector[i] ^ rest_vector[CONTROL_LENGTH - 1 - 1];
				if (result != 0) {
					for (int16_t j = CONTROL_LENGTH - 1 - 1; j > 0; j--)
						if (gen_poly_vector[j] != 0)
							rest_vector[j] = rest_vector[j - 1] ^ result;
						else
							rest_vector[j] = rest_vector[j - 1];
					rest_vector[0] = gen_poly_vector[0] && result;
				}
				else {
					for (int16_t j = CONTROL_LENGTH - 1 - 1; j > 0; j--)
						rest_vector[j] = rest_vector[j - 1];
					rest_vector[0] = 0;
				}
			}

			timespec_get(&ts, TIME_UTC);
			enc_stop = ts.tv_nsec;

			for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
				enc_vector[i] = rest_vector[i];

			for (uint8_t i = 0; i < DATA_LENGTH; i++)
				enc_vector[i + CONTROL_LENGTH - 1] = data_vector[i];

			/* Add errors */
			memcpy(recv_vector, enc_vector, sizeof(recv_vector));

			if (err_amount > 0) {
				/* Random spread errors */
				if (err_type == 0) {
					for (uint8_t i = 0; i < err_amount; i++) {
						rand_err_location[i] = rand() % CODE_LENGTH;
						recv_vector[rand_err_location[i]] = 1 - recv_vector[rand_err_location[i]];
					}
				}
				/* Random condensed errors */
				else if (err_type == 1) {
					uint16_t init_err_location = 0;
					init_err_location = rand() % CODE_LENGTH;
					for (uint8_t i = 0; i < err_amount; i++) {
						rand_err_location[i] = (i + init_err_location) % CODE_LENGTH;
						recv_vector[rand_err_location[i]] = 1 - recv_vector[rand_err_location[i]];
					}
				}
			}

			/* Decoding data (simple)*/
			memcpy(shifted_recv_vector, recv_vector, sizeof(shifted_recv_vector));

			timespec_get(&ts, TIME_UTC);
			dec_start = ts.tv_nsec;

			while (1) {
				memset(recv_rest_vector, 0, sizeof(recv_rest_vector));
				for (int16_t i = CODE_LENGTH - 1; i >= 0; i--) {
					result = shifted_recv_vector[i] ^ recv_rest_vector[CODE_LENGTH - DATA_LENGTH - 1];
					if (result != 0) {
						for (int16_t j = CODE_LENGTH - DATA_LENGTH - 1; j > 0; j--)
							if (gen_poly_vector[j] != 0)
								recv_rest_vector[j] = recv_rest_vector[j - 1] ^ result;
							else
								recv_rest_vector[j] = recv_rest_vector[j - 1];
						recv_rest_vector[0] = gen_poly_vector[0] && result;
					}
					else {
						for (int16_t j = CODE_LENGTH - DATA_LENGTH - 1; j > 0; j--)
							recv_rest_vector[j] = recv_rest_vector[j - 1];
						recv_rest_vector[0] = 0;
					}
				}

				weight = 0;
				for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
					weight += recv_rest_vector[i];

				if (weight == 0)
					break;

				if (shift >= CODE_LENGTH) {
					err_detected = 1;
					dec_err = 1;
					break;
				}

				if ((weight > 0) && (weight <= ERR_CORR_CAPABILITY)) {
					err_detected = 1;
					break;
				}

				if ((weight > ERR_CORR_CAPABILITY) && (shift < CODE_LENGTH)) {
					uint16_t tmp = 0;
					shift++;
					tmp = shifted_recv_vector[0];
					for (uint16_t i = 0; i < CODE_LENGTH - 1; i++)
					{
						shifted_recv_vector[i] = shifted_recv_vector[i + 1];
					}
					shifted_recv_vector[CODE_LENGTH - 1] = tmp;
				}
			}

			if ((weight > 0) && (shift == 0)) {
				memcpy(repaired_recv_vector, shifted_recv_vector, sizeof(repaired_recv_vector));

				uint16_t counter = 0;
				for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
					if (recv_rest_vector[i] == 1) {
						repaired_recv_vector[i + DATA_LENGTH] = 1 - repaired_recv_vector[i + DATA_LENGTH];
						err_loc[0 + counter] = i + DATA_LENGTH;
						counter++;
					}
			}

			if ((weight > 0) && (shift != 0) && (shift < CODE_LENGTH)) {
				uint16_t tmp_arr[CODE_LENGTH] = { 0 };
				uint16_t counter = 0;
				for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
					if (recv_rest_vector[i] == 1) {
						shifted_recv_vector[i + DATA_LENGTH] = 1 - shifted_recv_vector[i + DATA_LENGTH];
						err_loc[0 + counter] = (i + shift + DATA_LENGTH) % CODE_LENGTH;
						counter++;
					}

				memcpy(tmp_arr, shifted_recv_vector, sizeof(tmp_arr));

				for (uint16_t i = 0; i < CODE_LENGTH - shift; i++)
					repaired_recv_vector[i + shift] = tmp_arr[i];

				uint16_t tmp = 0;
				for (uint16_t i = CODE_LENGTH - shift; i < CODE_LENGTH; i++) {
					repaired_recv_vector[0 + tmp] = tmp_arr[i];
					tmp++;
				}
			}

			timespec_get(&ts, TIME_UTC);
			dec_stop = ts.tv_nsec;

			timespec_get(&ts, TIME_UTC);
			exec_stop = ts.tv_nsec;

			enc_times[b] = (double)(enc_stop - enc_start) / 0xf4240;
			dec_times[b] = (double)(dec_stop - dec_start) / 0xf4240;
			exec_times[b] = (double)(exec_stop - exec_start) / 0xf4240;

			if (benchmark == 1 && csv == 0) {
				if (b == 0)
					fprintf(log, "=================================");
				else 
					fprintf(log, "\n=================================");
				fprintf(log, "\nLoop #%d.", b + 1);
				fprintf(log, "\nEncoding time: %f ms", enc_times[b]);
				fprintf(log, "\nDecoding time: %f ms", dec_times[b]);
				fprintf(log, "\nExecution time: %f ms", exec_times[b]);

				if (err_amount > 0) {
					fprintf(log, "\nRandom error locations: ");
					for (uint8_t i = 0; i < err_amount; i++) {
						if ((i > 1) && (i % (log_trim / 5) == 0))
							fprintf(log, "\n");
						fprintf(log, "%d", rand_err_location[i]);
						if (i != err_amount - 1)
							fprintf(log, ", ");
					}
				}
				else {
					fprintf(log, "\nNo errors added.");
				}

				if (err_detected == 0) {
					fprintf(log, "\nNo errors detected.");
				}
				else if (err_detected != 0 && shift < CODE_LENGTH) {
					fprintf(log, "\nErrors detected and repaired.");
				}
				else if (dec_err == 1) {
					fprintf(log, "\nDecoding error, unrepairable errors.");
				}
				if (b == bench_loops - 1) {
					double enc_sum = 0;
					double dec_sum = 0;
					double exec_sum = 0;
					for (uint16_t i = 0; i < bench_loops; i++) {
						enc_sum += enc_times[i];
						dec_sum += dec_times[i];
						exec_sum += exec_times[i];
					}
					fprintf(log, "\n=================================");
					fprintf(log, "\nAverage encoding time: %f ms", enc_sum / bench_loops);
					fprintf(log, "\nAverage decoding time: %f ms", dec_sum / bench_loops);
					fprintf(log, "\nAverage execution time: %f ms", exec_sum / bench_loops);
				}
			}
			if (csv == 1) {
				if (b == 0) {
					fprintf(log, "loop,enc_time,dec_time,exec_time,success,");
					for (uint8_t i = 0; i < err_amount; i++) {
						if (i == err_amount - 1)
							fprintf(log, "e%d", i + 1);
						else
							fprintf(log, "e%d,", i + 1);
					}
				}
				fprintf(log, "\n%d,", b + 1);
				fprintf(log, "%f,", enc_times[b]);
				fprintf(log, "%f,", dec_times[b]);
				fprintf(log, "%f,", exec_times[b]);
				if (err_detected == 0) {
					fprintf(log, "0,");
				}
				else if (err_detected != 0 && shift < CODE_LENGTH) {
					fprintf(log, "1,");
				}
				else if (dec_err == 1) {
					fprintf(log, "2,");
				}
				for (uint8_t i = 0; i < err_amount; i++) {
					if (i == err_amount - 1)
						fprintf(log, "%d", rand_err_location[i]);
					else
						fprintf(log, "%d,", rand_err_location[i]);
				}
			}
			b++;
		} while (b < bench_loops);

		if (benchmark == 0) {
			fprintf(log, "Generator polynomial vector:\n");
			for (uint16_t i = 0; i <= CONTROL_LENGTH - 1; i++) {
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", gen_poly_vector[i]);
			}

			fprintf(log, "\n\nRandom data vector:\n");
			for (uint8_t i = 0; i < DATA_LENGTH; i++) {
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", data_vector[i]);
			}

			fprintf(log, "\n\nRest vector:\n");
			for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++) {
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", rest_vector[i]);
			}

			fprintf(log, "\n\nEncoded vector:\n");
			for (uint16_t i = 0; i < CODE_LENGTH; i++) {
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", enc_vector[i]);
			}

			fprintf(log, "\n\nEncoding time: %f ms", (double)(enc_stop - enc_start) / 0xf4240);

			if (err_amount > 0) {
				fprintf(log, "\n\nRandom error locations (defined):\n");
				for (uint8_t i = 0; i < err_amount; i++) {
					if ((i > 1) && (i % (log_trim / 5) == 0))
						fprintf(log, "\n");
					fprintf(log, "%d", rand_err_location[i]);
					if (i != err_amount - 1)
						fprintf(log, ", ");
				}
			}
			else {
				fprintf(log, "\n\nNo errors added.");
			}

			fprintf(log, "\n\nReceived vector:\n");
			for (uint16_t i = 0; i < CODE_LENGTH; i++) {
				uint8_t highlight = 0;
				for (uint8_t j = 0; j < weight; j++)
					if (i == err_loc[j])
						highlight = 1;
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				if ((global_highlight == 1) && (highlight == 1) && (dec_err != 1))
					fprintf(log, "[");
				fprintf(log, "%d", recv_vector[i]);
				if ((global_highlight == 1) && (highlight == 1) && (dec_err != 1))
					fprintf(log, "]");
			}

			fprintf(log, "\n\nReceived vector was shifted %d times.", shift);

			fprintf(log, "\n\nReceived rest vector:\n");
			for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++) {
				if ((i > 1) && (i % log_trim == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", recv_rest_vector[i]);
			}

			if (err_detected == 0) {
				fprintf(log, "\n\nNo errors detected.");
			}
			else if (err_detected != 0 && shift < CODE_LENGTH) {
				fprintf(log, "\n\nErrors positions:\n");
				for (uint8_t i = 0; i < weight; i++) {
					if ((i > 1) && (i % (log_trim / 5) == 0))
						fprintf(log, "\n");
					fprintf(log, "%d", err_loc[i]);
					if (i != weight - 1)
						fprintf(log, ", ");
				}

				fprintf(log, "\n\n%d errors detected and repaired.\n\nRepaired received vector:\n", weight);
				for (uint16_t i = 0; i < CODE_LENGTH; i++) {
					uint8_t highlight = 0;
					for (uint8_t j = 0; j < weight; j++)
						if (i == err_loc[j])
							highlight = 1;
					if ((i > 1) && (i % log_trim == 0))
						fprintf(log, "\n");
					if ((global_highlight == 1) && (highlight == 1))
						fprintf(log, "[");
					fprintf(log, "%d", repaired_recv_vector[i]);
					if ((global_highlight == 1) && (highlight == 1))
						fprintf(log, "]");
				}
			}
			else if (dec_err == 1) {
				fprintf(log, "\n\nDecoding error, unrepairable errors.\n");
			}

			fprintf(log, "\n\nDecoding time: %f ms", (double)(dec_stop - dec_start) / 0xf4240);

			fprintf(log, "\n\nExecution time: %f ms", (double)(exec_stop - exec_start) / 0xf4240);
		}
	}
}
