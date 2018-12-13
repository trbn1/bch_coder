#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIELD_DEGREE 0xa
#define DATA_LENGTH 0xdc
#define CODE_LENGTH 0x1ff
#define CONTROL_LENGTH 0x124
#define ERR_CORR_CAPABILITY 0x27
#define ERR_AMOUNT 0x4
#define LOG_TRIM 0x32

int main(void)
{
	static const unsigned int ERR_TYPE = 0x0;
	/* Initializating generator and parameters */
	static const unsigned int field = 0x2;
	static const unsigned int field_degree = FIELD_DEGREE - 1;
	static const unsigned int code_length = CODE_LENGTH;
	static const unsigned int data_length = DATA_LENGTH;
	static const unsigned int err_corr_capability = ERR_CORR_CAPABILITY;
	static const unsigned int min_distance = 0x4f;
	static const unsigned int control_length = CONTROL_LENGTH - 1;

	static const unsigned int prim_poly_tab[] = {
		0x0, 0x1, 0x5, 0x7, 0x9,
	};

	static const unsigned int gen_poly_tab[] = {
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

	unsigned int prim_poly_vector[FIELD_DEGREE] = { 0 };
	unsigned int gen_poly_vector[CONTROL_LENGTH] = { 0 };

	size_t prim_poly_tab_size = sizeof(prim_poly_tab) / sizeof(prim_poly_tab[0]);
	size_t gen_poly_tab_size = sizeof(gen_poly_tab) / sizeof(gen_poly_tab[0]);

	for (int i = 0; i < prim_poly_tab_size; i++)
		prim_poly_vector[prim_poly_tab[i]] = 1;

	for (int i = 0; i < gen_poly_tab_size; i++)
		gen_poly_vector[gen_poly_tab[i]] = 1;

	/* Generating random data */
	srand( (unsigned int)time(NULL) );
	unsigned int data_vector[DATA_LENGTH] = { 0 };

	for (unsigned int i = 0; i < data_length; i++)
		data_vector[i] = rand() % 2;

	/* Encoding data */
	unsigned int rest_vector[CONTROL_LENGTH] = { 0 };
	unsigned int result = 0;

		for (int i = data_length - 1; i >= 0; i--) {
			result = data_vector[i] ^ rest_vector[control_length - 1];
			if (result != 0) {
				for (int j = control_length - 1; j > 0; j--)
					if (gen_poly_vector[j] != 0)
						rest_vector[j] = rest_vector[j - 1] ^ result;
					else
						rest_vector[j] = rest_vector[j - 1];
				rest_vector[0] = gen_poly_vector[0] && result;
			}
			else {
				for (int j = control_length - 1; j > 0; j--)
					rest_vector[j] = rest_vector[j - 1];
				rest_vector[0] = 0;
			}
		}

	unsigned int enc_vector[CODE_LENGTH] = { 0 };

	for (unsigned int i = 0; i < control_length; i++)
		enc_vector[i] = rest_vector[i];

	for (unsigned int i = 0; i < data_length; i++)
		enc_vector[i + control_length] = data_vector[i];

	/* Decoding data (simple)*/
	unsigned int recv_vector[CODE_LENGTH] = { 0 };
	unsigned int recv_rest_vector[CONTROL_LENGTH] = { 0 };
	unsigned int shift = 0;
	unsigned int err_detected = 0;
	unsigned int dec_err = 0;
	result = 0;

	memcpy(recv_vector, enc_vector, sizeof(recv_vector));

	unsigned int rand_err_location[ERR_AMOUNT] = { 0 };
	/* Random spread errors */
	if (ERR_TYPE == 0) {
		for (unsigned int i = 0; i < ERR_AMOUNT; i++) {
			rand_err_location[i] = rand() % code_length;
			recv_vector[rand_err_location[i]] = 1 - recv_vector[rand_err_location[i]];
		}
	}
	/* Random condensed errors */
	else if (ERR_TYPE == 1) {
		unsigned int init_err_location = 0;
		init_err_location = rand() % code_length;
		for (unsigned int i = 0; i < ERR_AMOUNT; i++) {
			rand_err_location[i] = (i + init_err_location) % code_length;
			recv_vector[rand_err_location[i]] = 1 - recv_vector[rand_err_location[i]];
		}
	}

	unsigned int shifted_recv_vector[CODE_LENGTH] = { 0 };
	memcpy(shifted_recv_vector, recv_vector, sizeof(shifted_recv_vector));

	unsigned int weight = 0;
	while (1) {
		memset(recv_rest_vector, 0, sizeof(recv_rest_vector));
		for (int i = code_length - 1; i >= 0; i--) {
			result = shifted_recv_vector[i] ^ recv_rest_vector[code_length - data_length - 1];
			if (result != 0) {
				for (int j = code_length - data_length - 1; j > 0; j--)
					if (gen_poly_vector[j] != 0)
						recv_rest_vector[j] = recv_rest_vector[j - 1] ^ result;
					else
						recv_rest_vector[j] = recv_rest_vector[j - 1];
				recv_rest_vector[0] = gen_poly_vector[0] && result;
			}
			else {
				for (int j = code_length - data_length - 1; j > 0; j--)
					recv_rest_vector[j] = recv_rest_vector[j - 1];
				recv_rest_vector[0] = 0;
			}
		}

		weight = 0;
		for (unsigned int i = 0; i < control_length; i++)
			weight += recv_rest_vector[i];

		if (weight == 0)
			break;

		if (shift >= CODE_LENGTH) {
			err_detected = 1;
			dec_err = 1;
			break;
		}

		if ((weight > 0) && (weight <= err_corr_capability)) {
			err_detected = 1;
			break;
		}

		if ((weight > err_corr_capability) && (shift < CODE_LENGTH)) {
			unsigned int tmp = 0;
			shift++;
			tmp = shifted_recv_vector[0];
			for (unsigned int i = 0; i < code_length - 1; i++)
			{
				shifted_recv_vector[i] = shifted_recv_vector[i + 1];
			}
			shifted_recv_vector[code_length - 1] = tmp;
		}
	}

	unsigned int repaired_recv_vector[CODE_LENGTH] = { 0 };
	unsigned int err_loc[ERR_CORR_CAPABILITY] = { 0 };
	if ((weight > 0) && (shift == 0)) {
		memcpy(repaired_recv_vector, shifted_recv_vector, sizeof(repaired_recv_vector));

		unsigned int counter = 0;
		for (unsigned int i = 0; i < control_length; i++)
			if (recv_rest_vector[i] == 1) {
				repaired_recv_vector[i + DATA_LENGTH] = 1 - repaired_recv_vector[i + DATA_LENGTH];
				err_loc[0 + counter] = i + DATA_LENGTH;
				counter++;
			}
	}

	if ((weight > 0) && (shift != 0) && (shift < CODE_LENGTH)) {
		unsigned int tmp_arr[CODE_LENGTH] = { 0 };
		unsigned int counter = 0;
		for (unsigned int i = 0; i < control_length; i++)
			if (recv_rest_vector[i] == 1) {
				shifted_recv_vector[i + DATA_LENGTH] = 1 - shifted_recv_vector[i + DATA_LENGTH];
				err_loc[0 + counter] = (i + shift + DATA_LENGTH) % CODE_LENGTH;
				counter++;
			}

		memcpy(tmp_arr, shifted_recv_vector, sizeof(tmp_arr));

		for (unsigned int i = 0; i < code_length - shift; i++)
			repaired_recv_vector[i + shift] = tmp_arr[i];

		unsigned int tmp = 0;
		for (unsigned int i = code_length - shift; i < code_length; i++) {
			repaired_recv_vector[0 + tmp] = tmp_arr[i];
			tmp++;
		}
	}
	


	FILE *log;
	errno_t err;
	if ((err = fopen_s(&log, "out.log", "w")) != 0) {
		return 1;
	}
	else {

		fprintf(log, "Primitive polynomial vector:\n");
		for (unsigned int i = 0; i <= field_degree; i++)
			fprintf(log, "%d", prim_poly_vector[i]);

		fprintf(log, "\n\nGenerator polynomial vector:\n");
		for (unsigned int i = 0; i <= control_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", gen_poly_vector[i]);
		}

		fprintf(log, "\n\nRandom data vector:\n");
		for (unsigned int i = 0; i < data_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", data_vector[i]);
		}

		fprintf(log, "\n\nRest vector:\n");
		for (unsigned int i = 0; i < control_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", rest_vector[i]);
		}

		fprintf(log, "\n\nEncoded vector:\n");
		for (unsigned int i = 0; i < code_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", enc_vector[i]);
		}

		fprintf(log, "\n\nRandom error locations (defined):\n");
		for (unsigned int i = 0; i < ERR_AMOUNT; i++) {
			if ((i > 1) && (i % (LOG_TRIM / 5) == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", rand_err_location[i]);
			if (i != ERR_AMOUNT - 1)
				fprintf(log, ", ");
		}

		fprintf(log, "\n\nReceived vector:\n");
		for (unsigned int i = 0; i < code_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", recv_vector[i]);
		}

		fprintf(log, "\n\nReceived vector was shifted %d times.", shift);

		fprintf(log, "\n\nReceived rest vector:\n");
		for (unsigned int i = 0; i < control_length; i++) {
			if ((i > 1) && (i % LOG_TRIM == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", recv_rest_vector[i]);
		}

		if (err_detected == 0) {
			fprintf(log, "\n\nNo errors detected.\n");
		}
		else if (err_detected != 0 && shift < CODE_LENGTH) {
			fprintf(log, "\n\nErrors positions:\n");
			for (unsigned int i = 0; i < weight; i++) {
				if ((i > 1) && (i % (LOG_TRIM / 5) == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", err_loc[i]);
				if (i != weight - 1)
					fprintf(log, ", ");
			}

			fprintf(log, "\n\n%d errors detected and repaired.\n\nRepaired received vector:\n", weight);
			for (unsigned int i = 0; i < code_length; i++) {
				if ((i > 1) && (i % LOG_TRIM == 0))
					fprintf(log, "\n");
				fprintf(log, "%d", repaired_recv_vector[i]);
			}
		}
		else if (dec_err == 1) {
			fprintf(log, "\n\nDecoding error, too many errors.\n");
		}
	}
	return 0;
}
