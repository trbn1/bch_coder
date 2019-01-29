#include "headers/bch.h"
#include <stdio.h>

void generate_field(const uint8_t *prim_poly_tab, size_t prim_poly_tab_size, struct field_elements *fe) {
	/* Load primary polynomial into array */
	for (uint8_t i = 0; i < prim_poly_tab_size; i++) {
		fe->prim_poly_vector[prim_poly_tab[i]] = 1;
	}

	/* Calculate field elements */
	uint16_t mask = 1;
	fe->alpha[FIELD_DEGREE] = 0;
	for (uint16_t i = 0; i < FIELD_DEGREE; i++) {
		fe->alpha[i] = mask;
		fe->index[fe->alpha[i]] = i;
		if (fe->prim_poly_vector[i] != 0)
			fe->alpha[FIELD_DEGREE] ^= mask;
		mask <<= 1;
	}
	fe->index[fe->alpha[FIELD_DEGREE]] = FIELD_DEGREE;

	mask >>= 1;
	for (uint16_t i = FIELD_DEGREE + 1; i < CODE_LENGTH; i++) {
		if (fe->alpha[i - 1] >= mask)
			fe->alpha[i] = fe->alpha[FIELD_DEGREE] ^ ((fe->alpha[i - 1] ^ mask) << 1);
		else
			fe->alpha[i] = fe->alpha[i - 1] << 1;
		fe->index[fe->alpha[i]] = i;
	}
	fe->index[0] = -1;
}

void init_generator(const uint16_t *gen_poly_tab, size_t gen_poly_tab_size, struct bch_enc *bch_enc, struct bch_sdec *bch_sdec) {
	/* Load generator polynomial into array */
	for (uint8_t i = 0; i < gen_poly_tab_size; i++) {
		bch_enc->gen_poly_vector[gen_poly_tab[i]] = 1;
	}
	memcpy(bch_sdec->gen_poly_vector, bch_enc->gen_poly_vector, sizeof(bch_sdec->gen_poly_vector));
}

void reset_bch_enc(struct bch_enc *bch) {
	/* Reset variables */
	memset(bch->data_vector, 0, sizeof(bch->data_vector));
	memset(bch->rest_vector, 0, sizeof(bch->rest_vector));
	memset(bch->enc_vector, 0, sizeof(bch->enc_vector));
	bch->result = 0;
}

void reset_bch_sdec(struct bch_sdec *bch) {
	/* Reset variables */
	memset(bch->recv_vector, 0, sizeof(bch->recv_vector));
	memset(bch->recv_rest_vector, 0, sizeof(bch->recv_rest_vector));
	memset(bch->rand_err_location, 0, sizeof(bch->rand_err_location));
	memset(bch->shifted_recv_vector, 0, sizeof(bch->shifted_recv_vector));
	memset(bch->repaired_recv_vector, 0, sizeof(bch->repaired_recv_vector));
	memset(bch->err_loc, 0, sizeof(bch->err_loc));
	bch->result = 0;
	bch->shift = 0;
	bch->err_detected = 0;
	bch->dec_err = 0;
	bch->weight = 0;
}

void reset_bch_dec(struct bch_dec *bch) {
	/* Reset variables */
	memset(bch->recv_vector, 0, sizeof(bch->recv_vector));
	memset(bch->syndromes, 0, sizeof(bch->syndromes));
	memset(bch->elp, 0, sizeof(bch->elp));
	memset(bch->d, 0, sizeof(bch->d));
	memset(bch->l, 0, sizeof(bch->l));
	memset(bch->u_lu, 0, sizeof(bch->u_lu));
	memset(bch->root, 0, sizeof(bch->root));
	memset(bch->loc, 0, sizeof(bch->loc));
	memset(bch->reg, 0, sizeof(bch->reg));
	bch->err_detected = 0;
	bch->dec_err = 0;
}

void generate_random_data(struct bch_enc *bch) {
	/* Generate random data (0s and 1s) into array */
	for (uint8_t i = 0; i < DATA_LENGTH; i++)
		bch->data_vector[i] = rand() % 2;
}

void encode_data(struct bch_enc *bch) {
	/* Encode data */
	for (int16_t i = DATA_LENGTH - 1; i >= 0; i--) {
		bch->result = bch->data_vector[i] ^ bch->rest_vector[CONTROL_LENGTH - 1 - 1];
		if (bch->result != 0) {
			for (int16_t j = CONTROL_LENGTH - 1 - 1; j > 0; j--)
				if (bch->gen_poly_vector[j] != 0)
					bch->rest_vector[j] = bch->rest_vector[j - 1] ^ bch->result;
				else
					bch->rest_vector[j] = bch->rest_vector[j - 1];
			bch->rest_vector[0] = bch->gen_poly_vector[0] && bch->result;
		}
		else {
			memmove(&bch->rest_vector[1], &bch->rest_vector[0], sizeof(bch->rest_vector) - sizeof(*bch->rest_vector));
			bch->rest_vector[0] = 0;
		}
	}
	memcpy(bch->enc_vector, bch->rest_vector, sizeof(bch->rest_vector));
	memcpy(bch->enc_vector + CONTROL_LENGTH - 1, bch->data_vector, sizeof(bch->data_vector));
}

void receive_encoded_data(struct bch_enc *bch_enc, struct bch_sdec *bch_sdec, struct bch_dec *bch_dec) {
	/* Copy encoded data to simulate receiving */
	memcpy(bch_sdec->recv_vector, bch_enc->enc_vector, sizeof(bch_sdec->recv_vector));
	memcpy(bch_dec->recv_vector, bch_enc->enc_vector, sizeof(bch_dec->recv_vector));
}

void add_errors(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct user_settings *us) {
	/* Add randomly spread errors */
	if (us->err_type == 0) {
		for (uint8_t i = 0; i < us->err_amount; i++) {
			/* Dont allow duplicate error locations */
			uint8_t counter = 0;
			while (1) {
				bch_sdec->rand_err_location[i] = rand() % CODE_LENGTH;
				for (uint8_t j = 0; j < us->err_amount; j++) {
					if (bch_sdec->rand_err_location[i] != bch_sdec->rand_err_location[j])
						counter++;
				}
				if (counter == us->err_amount - 1)
					break;
			}
			bch_sdec->recv_vector[bch_sdec->rand_err_location[i]] = 1 - bch_sdec->recv_vector[bch_sdec->rand_err_location[i]];
		}
	}
	/* Add random condensed errors */
	else if (us->err_type == 1) {
		uint16_t init_err_location = 0;
		init_err_location = rand() % CODE_LENGTH;
		for (uint8_t i = 0; i < us->err_amount; i++) {
			bch_sdec->rand_err_location[i] = (i + init_err_location) % CODE_LENGTH;
			bch_sdec->recv_vector[bch_sdec->rand_err_location[i]] = 1 - bch_sdec->recv_vector[bch_sdec->rand_err_location[i]];
		}
	}
	memcpy(bch_dec->recv_vector, bch_sdec->recv_vector, sizeof(bch_dec->recv_vector));
}

void sdecode_data(struct bch_sdec *bch) {
	/* Simplified decoding */
	while (1) {
		/* https://stackoverflow.com/a/171309 */
		memset(bch->recv_rest_vector, 0, sizeof(bch->recv_rest_vector));
		for (int16_t i = CODE_LENGTH - 1; i >= 0; i--) {
			bch->result = bch->shifted_recv_vector[i] ^ bch->recv_rest_vector[CODE_LENGTH - DATA_LENGTH - 1];
			if (bch->result != 0) {
				for (int16_t j = CODE_LENGTH - DATA_LENGTH - 1; j > 0; j--)
					if (bch->gen_poly_vector[j] != 0)
						bch->recv_rest_vector[j] = bch->recv_rest_vector[j - 1] ^ bch->result;
					else
						bch->recv_rest_vector[j] = bch->recv_rest_vector[j - 1];
				bch->recv_rest_vector[0] = bch->gen_poly_vector[0] && bch->result;
			}
			else {
				memmove(&bch->recv_rest_vector[1], &bch->recv_rest_vector[0], sizeof(bch->recv_rest_vector) - sizeof(*bch->recv_rest_vector));
				bch->recv_rest_vector[0] = 0;
			}
		}

		bch->weight = 0;
		for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
			bch->weight += bch->recv_rest_vector[i];

		if (bch->weight == 0)
			break;

		if (bch->shift == CODE_LENGTH) {
			bch->err_detected = 1;
			bch->dec_err = 1;
			break;
		}

		if ((bch->weight > 0) && (bch->weight <= ERR_CORR_CAPABILITY)) {
			bch->err_detected = 1;
			break;
		}

		if ((bch->weight > ERR_CORR_CAPABILITY) && (bch->shift < CODE_LENGTH)) {
			uint16_t tmp = 0;
			bch->shift++;
			tmp = bch->shifted_recv_vector[0];
			memmove(&bch->shifted_recv_vector[0], &bch->shifted_recv_vector[1], sizeof(bch->shifted_recv_vector) - sizeof(*bch->shifted_recv_vector));
			bch->shifted_recv_vector[CODE_LENGTH - 1] = tmp;
		}
	}

	if ((bch->weight > 0) && (bch->shift == 0)) {
		memcpy(bch->repaired_recv_vector, bch->shifted_recv_vector, sizeof(bch->repaired_recv_vector));

		uint16_t counter = 0;
		for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
			if (bch->recv_rest_vector[i] == 1) {
				bch->repaired_recv_vector[i + DATA_LENGTH] = 1 - bch->repaired_recv_vector[i + DATA_LENGTH];
				bch->err_loc[0 + counter] = i + DATA_LENGTH;
				counter++;
			}
	}

	if ((bch->weight > 0) && (bch->shift != 0) && (bch->shift < CODE_LENGTH)) {
		uint16_t tmp_arr[CODE_LENGTH] = { 0 };
		uint16_t counter = 0;
		for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
			if (bch->recv_rest_vector[i] == 1) {
				bch->shifted_recv_vector[i + DATA_LENGTH] = 1 - bch->shifted_recv_vector[i + DATA_LENGTH];
				bch->err_loc[0 + counter] = (i + bch->shift + DATA_LENGTH) % CODE_LENGTH;
				counter++;
			}

		memcpy(tmp_arr, bch->shifted_recv_vector, sizeof(tmp_arr));

		for (uint16_t i = 0; i < CODE_LENGTH - bch->shift; i++)
			bch->repaired_recv_vector[i + bch->shift] = tmp_arr[i];

		uint16_t tmp = 0;
		for (uint16_t i = CODE_LENGTH - bch->shift; i < CODE_LENGTH; i++) {
			bch->repaired_recv_vector[0 + tmp] = tmp_arr[i];
			tmp++;
		}
	}
}

void decode_data(struct field_elements *fe, struct bch_dec *bch) {
	/* Decoding data */
	for (uint16_t i = 1; i <= ERR_CORR_CAPABILITY * 2; i++) {
		/* Syndrome computation */
		bch->syndromes[i] = 0;
		for (uint16_t j = 0; j < CODE_LENGTH; j++)
			if (bch->recv_vector[j] != 0)
				bch->syndromes[i] ^= fe->alpha[(i * j) % CODE_LENGTH];
		if (bch->syndromes[i] != 0)
			bch->err_detected = 1;
		bch->syndromes[i] = fe->index[bch->syndromes[i]];
	}

	if (bch->err_detected) {
		/* Compute the error location polynomial using the Berlekamp iterative algorithm */
		/* Initialise entries */
		bch->d[0] = 0;
		bch->d[1] = bch->syndromes[1];
		bch->elp[0][0] = 0;
		bch->elp[1][0] = 1;
		for (uint16_t i = 1; i < ERR_CORR_CAPABILITY * 2; i++) {
			bch->elp[0][i] = -1;
			bch->elp[1][i] = 0;
		}
		bch->l[0] = 0;
		bch->l[1] = 0;
		bch->u_lu[0] = -1;
		bch->u_lu[1] = 0;

		uint16_t u = 0;
		do {
			u++;
			if (bch->d[u] == -1) {
				bch->l[u + 1] = bch->l[u];
				for (uint16_t i = 0; i <= bch->l[u]; i++) {
					bch->elp[u + 1][i] = bch->elp[u][i];
					bch->elp[u][i] = fe->index[bch->elp[u][i]];
				}
			}
			else
				/*
				 * search for words with greatest u_lu[q] for
				 * which d[q]!=0
				 */
			{
				uint16_t q = u - 1;
				while ((bch->d[q] == -1) && (q > 0))
					q--;
				/* have found first non-zero d[q]  */
				if (q > 0) {
					uint16_t j = q;
					do {
						j--;
						if ((bch->d[j] != -1) && (bch->u_lu[q] < bch->u_lu[j]))
							q = j;
					} while (j > 0);
				}

				/*
				 * have now found q such that d[u]!=0 and
				 * u_lu[q] is maximum
				 */
				 /* store degree of new elp polynomial */
				if (bch->l[u] > bch->l[q] + u - q)
					bch->l[u + 1] = bch->l[u];
				else
					bch->l[u + 1] = bch->l[q] + u - q;

				/* form new elp(x) */
				for (uint16_t i = 0; i < ERR_CORR_CAPABILITY * 2; i++)
					bch->elp[u + 1][i] = 0;
				for (uint16_t i = 0; i <= bch->l[q]; i++)
					if (bch->elp[q][i] != -1)
						bch->elp[u + 1][i + u - q] =
						fe->alpha[(bch->d[u] + CODE_LENGTH - bch->d[q] + bch->elp[q][i]) % CODE_LENGTH];
				for (uint16_t i = 0; i <= bch->l[u]; i++) {
					bch->elp[u + 1][i] ^= bch->elp[u][i];
					bch->elp[u][i] = fe->index[bch->elp[u][i]];
				}
			}
			bch->u_lu[u + 1] = u - bch->l[u + 1];

			/* form (u+1)th discrepancy */
			if (u < ERR_CORR_CAPABILITY * 2) {
				/* no discrepancy computed on last iteration */
				if (bch->syndromes[u + 1] != -1)
					bch->d[u + 1] = fe->alpha[bch->syndromes[u + 1]];
				else
					bch->d[u + 1] = 0;
				for (uint16_t i = 1; i <= bch->l[u + 1]; i++)
					if ((bch->syndromes[u + 1 - i] != -1) && (bch->elp[u + 1][i] != 0))
						bch->d[u + 1] ^= fe->alpha[(bch->syndromes[u + 1 - i]
							+ fe->index[bch->elp[u + 1][i]]) % CODE_LENGTH];
				/* put d[u+1] into index form */
				bch->d[u + 1] = fe->index[bch->d[u + 1]];
			}
		} while ((u < ERR_CORR_CAPABILITY * 2) && (bch->l[u + 1] <= ERR_CORR_CAPABILITY));

		u++;
		if (bch->l[u] <= ERR_CORR_CAPABILITY) {
			/* put elp into index form */
			for (uint16_t i = 0; i <= bch->l[u]; i++)
				bch->elp[u][i] = fe->index[bch->elp[u][i]];

			/* Chien search: find roots of the error location polynomial */
			for (uint16_t i = 1; i <= bch->l[u]; i++)
				bch->reg[i] = bch->elp[u][i];
			uint16_t count = 0;
			for (uint16_t i = 1; i <= CODE_LENGTH; i++) {
				uint16_t q = 1;
				for (uint16_t j = 1; j <= bch->l[u]; j++)
					if (bch->reg[j] != -1) {
						bch->reg[j] = (bch->reg[j] + j) % CODE_LENGTH;
						q ^= fe->alpha[bch->reg[j]];
					}
				if (!q) {	/* store root and error
						 * location number indices */
					bch->root[count] = i;
					bch->loc[count] = CODE_LENGTH - i;
					count++;
				}
			}

			if (count == bch->l[u])
				/* no. roots = degree of elp hence <= t errors */
				for (uint16_t i = 0; i < bch->l[u]; i++)
					bch->recv_vector[bch->loc[i]] ^= 1;
			else	/* elp has degree >t hence cannot solve */
				bch->dec_err = 1;
		}
	}
}