#ifndef BCH_H
#define BCH_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "const.h"

struct field_elements {
	uint8_t prim_poly_vector[FIELD_DEGREE + 1];
	uint16_t alpha[CODE_LENGTH + 1];
	int16_t index[CODE_LENGTH + 1];
};

struct bch_enc {
	uint16_t gen_poly_vector[CONTROL_LENGTH];

	uint16_t data_vector[DATA_LENGTH];
	uint16_t rest_vector[CONTROL_LENGTH];
	uint16_t result;
	uint16_t enc_vector[CODE_LENGTH];
};

struct bch_sdec {
	uint16_t gen_poly_vector[CONTROL_LENGTH];

	uint16_t recv_vector[CODE_LENGTH];
	uint16_t recv_rest_vector[CONTROL_LENGTH];
	uint16_t shift;
	uint16_t result;
	uint8_t err_detected;
	uint8_t dec_err;

	uint16_t rand_err_location[CODE_LENGTH];

	uint16_t shifted_recv_vector[CODE_LENGTH];
	uint16_t weight;

	uint16_t repaired_recv_vector[CODE_LENGTH];
	uint16_t err_loc[CODE_LENGTH];
};

struct bch_dec {
	uint16_t recv_vector[CODE_LENGTH];
	int16_t syndromes[ERR_CORR_CAPABILITY * 2 + 1];
	uint8_t err_detected;
	uint8_t dec_err;

	int16_t elp[ERR_CORR_CAPABILITY * 2 + 2][ERR_CORR_CAPABILITY * 2 + 2];
	int16_t d[ERR_CORR_CAPABILITY * 2 + 2];
	int16_t l[ERR_CORR_CAPABILITY * 2 + 2];
	int16_t u_lu[ERR_CORR_CAPABILITY * 2 + 2];
	int16_t root[ERR_CORR_CAPABILITY];
	int16_t loc[ERR_CORR_CAPABILITY];
	int16_t reg[ERR_CORR_CAPABILITY + 1];
};

void generate_field(const uint8_t *prim_poly_tab, size_t prim_poly_tab_size, struct field_elements *field_elements);

void init_generator(const uint16_t *gen_poly_tab, size_t gen_poly_tab_size, struct bch_enc *bch_enc, struct bch_sdec *bch_sdec);

void reset_bch_enc(struct bch_enc *bch);

void reset_bch_sdec(struct bch_sdec *bch);

void reset_bch_dec(struct bch_dec *bch);

void generate_random_data(struct bch_enc *bch);

void encode_data(struct bch_enc *bch);

void receive_encoded_data(struct bch_enc *bch_enc, struct bch_sdec *bch_sdec, struct bch_dec *bch_dec);

void add_errors(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct user_settings *us);

void sdecode_data(struct bch_sdec *bch);

void decode_data(struct field_elements *fe, struct bch_dec *bch);

#endif