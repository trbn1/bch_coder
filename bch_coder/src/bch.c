#include "headers/bch.h"

void init_generator(const uint16_t *gen_poly_tab, size_t gen_poly_tab_size, struct bch_enc *bch_enc, struct bch_sdec *bch_sdec) {
	for (uint8_t i = 0; i < gen_poly_tab_size; i++) {
		bch_enc->gen_poly_vector[gen_poly_tab[i]] = 1;
		//bch_sdec->gen_poly_vector[gen_poly_tab[i]] = 1;
	}
	memcpy(bch_sdec->gen_poly_vector, bch_enc->gen_poly_vector, sizeof(bch_sdec->gen_poly_vector));
}

void reset_bch_enc(struct bch_enc *bch) {
	memset(bch->data_vector, 0, sizeof(bch->data_vector));
	memset(bch->rest_vector, 0, sizeof(bch->rest_vector));
	memset(bch->enc_vector, 0, sizeof(bch->enc_vector));
	bch->result = 0;
}

void reset_bch_sdec(struct bch_sdec *bch) {
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

void generate_random_data(struct bch_enc *bch) {
	for (uint8_t i = 0; i < DATA_LENGTH; i++)
		bch->data_vector[i] = rand() % 2;
}

void encode_data(struct bch_enc *bch) {
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
			/*for (int16_t j = CONTROL_LENGTH - 1 - 1; j > 0; j--)
				bch->rest_vector[j] = bch->rest_vector[j - 1];*/
			bch->rest_vector[0] = 0;
		}
	}
	memcpy(bch->enc_vector, bch->rest_vector, sizeof(bch->rest_vector));
	memcpy(bch->enc_vector + CONTROL_LENGTH - 1, bch->data_vector, sizeof(bch->data_vector));
	/*for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++)
		bch->enc_vector[i] = bch->rest_vector[i];

	for (uint8_t i = 0; i < DATA_LENGTH; i++)
		bch->enc_vector[i + CONTROL_LENGTH - 1] = bch->data_vector[i];*/
}

void receive_encoded_data(struct bch_enc *bch_enc, struct bch_sdec *bch_sdec) {
	memcpy(bch_sdec->recv_vector, bch_enc->enc_vector, sizeof(bch_sdec->recv_vector));
}

void add_errors(struct bch_sdec *bch, struct user_settings *us) {
	/* Random spread errors */
	if (us->err_type == 0) {
		for (uint8_t i = 0; i < us->err_amount; i++) {
			bch->rand_err_location[i] = rand() % CODE_LENGTH;
			bch->recv_vector[bch->rand_err_location[i]] = 1 - bch->recv_vector[bch->rand_err_location[i]];
		}
	}
	/* Random condensed errors */
	else if (us->err_type == 1) {
		uint16_t init_err_location = 0;
		init_err_location = rand() % CODE_LENGTH;
		for (uint8_t i = 0; i < us->err_amount; i++) {
			bch->rand_err_location[i] = (i + init_err_location) % CODE_LENGTH;
			bch->recv_vector[bch->rand_err_location[i]] = 1 - bch->recv_vector[bch->rand_err_location[i]];
		}
	}
}

void sdecode_data(struct bch_sdec *bch) {
	while (1) {
		// https://stackoverflow.com/a/171309
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
				/*for (int16_t j = CODE_LENGTH - DATA_LENGTH - 1; j > 0; j--)
					bch->recv_rest_vector[j] = bch->recv_rest_vector[j - 1];*/
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
			/*for (uint16_t i = 0; i < CODE_LENGTH - 1; i++)
			{
				bch->shifted_recv_vector[i] = bch->shifted_recv_vector[i + 1];
			}*/
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