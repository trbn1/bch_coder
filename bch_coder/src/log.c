#include "headers/log.h"

void loop_print(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, uint16_t bench_iter, FILE *log) {
	if (bench_iter == 0)
		fprintf(log, "=================================");
	else
		fprintf(log, "\n=================================");
	fprintf(log, "\nLoop #%d.", bench_iter + 1);
	fprintf(log, "\nEncoding time: %.4f ms", timers->enc_times[bench_iter]);
	fprintf(log, "\nSimplified decoding time: %.4f ms", timers->sdec_times[bench_iter]);
	fprintf(log, "\nDecoding time: %.4f ms", timers->dec_times[bench_iter]);
	fprintf(log, "\nExecution time: %.4f ms", timers->exec_times[bench_iter]);
	fprintf(log, "\nErrors amount: %d", us->err_amount);

	if (us->err_amount > 0) {
		fprintf(log, "\nRandom error locations: (simple) ");
		for (uint8_t i = 0; i < us->err_amount; i++) {
			if ((i > 1) && (i % (us->log_trim / 5) == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", bch_sdec->rand_err_location[i]);
			if (i != us->err_amount - 1)
				fprintf(log, ", ");
		}

		fprintf(log, "\nRandom error locations: ");
		for (uint8_t i = 0; i < us->err_amount; i++) {
			if ((i > 1) && (i % (us->log_trim / 5) == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", bch_dec->loc[i]);
			if (i != us->err_amount - 1)
				fprintf(log, ", ");
		}
	}
	else {
		fprintf(log, "\nNo errors added.");
	}

	if (bch_sdec->err_detected == 0) {
		fprintf(log, "\nNo errors detected. (simple)");
	}
	else if (bch_sdec->err_detected != 0 && bch_sdec->shift < CODE_LENGTH) {
		fprintf(log, "\nErrors detected and repaired. (simple)");
	}
	else if (bch_sdec->dec_err == 1) {
		fprintf(log, "\nSimplified decoding error, unrepairable errors.");
	}

	if (bch_dec->err_detected == 0) {
		fprintf(log, "\nNo errors detected.");
	}
	else if (bch_dec->err_detected != 0 && bch_dec->dec_err == 0) {
		fprintf(log, "\nErrors detected and repaired.");
	}
	else if (bch_dec->dec_err == 1) {
		fprintf(log, "\nDecoding error, unrepairable errors.");
	}

	if (bench_iter == us->bench_loops - 1) {
		double enc_sum = 0;
		double sdec_sum = 0;
		double dec_sum = 0;
		double exec_sum = 0;
		for (uint16_t i = 0; i < us->bench_loops; i++) {
			enc_sum += timers->enc_times[i];
			sdec_sum += timers->sdec_times[i];
			dec_sum += timers->dec_times[i];
			exec_sum += timers->exec_times[i];
		}
		fprintf(log, "\n=================================");
		fprintf(log, "\nAverage encoding time: %.4f ms", enc_sum / us->bench_loops);
		fprintf(log, "\nAverage simplified decoding time: %.4f ms", sdec_sum / us->bench_loops);
		fprintf(log, "\nAverage decoding time: %.4f ms", dec_sum / us->bench_loops);
		fprintf(log, "\nAverage execution time: %.4f ms", exec_sum / us->bench_loops);
	}
}

void csv_print(struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, uint16_t bench_iter, FILE *log) {
	if (bench_iter == 0) {
		fprintf(log, "loop,enc_time,sdec_time,dec_time,exec_time,sdec_success,dec_success,err_amount,err_type");
	}
	fprintf(log, "\n%d,", bench_iter + 1);
	fprintf(log, "%.4f,", timers->enc_times[bench_iter]);
	fprintf(log, "%.4f,", timers->sdec_times[bench_iter]);
	fprintf(log, "%.4f,", timers->dec_times[bench_iter]);
	fprintf(log, "%.4f,", timers->exec_times[bench_iter]);

	if (bch_sdec->err_detected == 0) {
		fprintf(log, "0,");
	}
	else if (bch_sdec->err_detected != 0 && bch_sdec->shift < CODE_LENGTH) {
		fprintf(log, "1,");
	}
	else if (bch_sdec->dec_err == 1) {
		fprintf(log, "2,");
	}

	if (bch_dec->err_detected == 0) {
		fprintf(log, "0,");
	}
	else if (bch_dec->err_detected != 0 && bch_dec->dec_err == 0) {
		fprintf(log, "1,");
	}
	else if (bch_dec->dec_err == 1) {
		fprintf(log, "2,");
	}

	fprintf(log, "%d,", us->err_amount);
	fprintf(log, "%d", us->err_type);
}

void full_print(struct bch_enc *bch_enc, struct bch_sdec *bch_sdec, struct bch_dec *bch_dec, struct timers *timers, struct user_settings *us, FILE *log) {
	fprintf(log, "=====================Encoding=====================");
	fprintf(log, "\n\nGenerator polynomial vector:\n");
	for (uint16_t i = 0; i <= CONTROL_LENGTH - 1; i++) {
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_enc->gen_poly_vector[i]);
	}

	fprintf(log, "\n\nRandom data vector:\n");
	for (uint8_t i = 0; i < DATA_LENGTH; i++) {
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_enc->data_vector[i]);
	}

	fprintf(log, "\n\nRest vector:\n");
	for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++) {
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_enc->rest_vector[i]);
	}

	fprintf(log, "\n\nEncoded vector:\n");
	for (uint16_t i = 0; i < CODE_LENGTH; i++) {
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_enc->enc_vector[i]);
	}

	fprintf(log, "\n\nEncoding time: %.4f ms", (double)(timers->enc_stop - timers->enc_start) / 0xf4240);

	if (us->err_amount > 0) {
		fprintf(log, "\n\nRandom error locations (defined):\n");
		for (uint8_t i = 0; i < us->err_amount; i++) {
			if ((i > 1) && (i % (us->log_trim / 5) == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", bch_sdec->rand_err_location[i]);
			if (i != us->err_amount - 1)
				fprintf(log, ", ");
		}
	}
	else {
		fprintf(log, "\n\nNo errors added.");
	}

	fprintf(log, "\n\nReceived vector:\n");
	for (uint16_t i = 0; i < CODE_LENGTH; i++) {
		uint8_t highlight = 0;
		for (uint8_t j = 0; j < bch_sdec->weight; j++)
			if (i == bch_sdec->err_loc[j])
				highlight = 1;
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		if ((us->global_highlight == 1) && (highlight == 1) && (bch_sdec->dec_err != 1))
			fprintf(log, "[");
		fprintf(log, "%d", bch_sdec->recv_vector[i]);
		if ((us->global_highlight == 1) && (highlight == 1) && (bch_sdec->dec_err != 1))
			fprintf(log, "]");
	}

	fprintf(log, "\n\n===============Simplified decoding================");
	fprintf(log, "\n\nReceived vector was shifted %d times.", bch_sdec->shift);

	fprintf(log, "\n\nReceived rest vector:\n");
	for (uint16_t i = 0; i < CONTROL_LENGTH - 1; i++) {
		if ((i > 1) && (i % us->log_trim == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_sdec->recv_rest_vector[i]);
	}

	if (bch_sdec->err_detected == 0) {
		fprintf(log, "\n\nNo errors detected.");
	}
	else if (bch_sdec->err_detected != 0 && bch_sdec->shift < CODE_LENGTH) {
		fprintf(log, "\n\nErrors positions:\n");
		for (uint8_t i = 0; i < bch_sdec->weight; i++) {
			if ((i > 1) && (i % (us->log_trim / 5) == 0))
				fprintf(log, "\n");
			fprintf(log, "%d", bch_sdec->err_loc[i]);
			if (i != bch_sdec->weight - 1)
				fprintf(log, ", ");
		}

		fprintf(log, "\n\n%d errors detected and repaired.\n\nRepaired received vector:\n", bch_sdec->weight);
		for (uint16_t i = 0; i < CODE_LENGTH; i++) {
			uint8_t highlight = 0;
			for (uint8_t j = 0; j < bch_sdec->weight; j++)
				if (i == bch_sdec->err_loc[j])
					highlight = 1;
			if ((i > 1) && (i % us->log_trim == 0))
				fprintf(log, "\n");
			if ((us->global_highlight == 1) && (highlight == 1))
				fprintf(log, "[");
			fprintf(log, "%d", bch_sdec->repaired_recv_vector[i]);
			if ((us->global_highlight == 1) && (highlight == 1))
				fprintf(log, "]");
		}
	}
	else if (bch_sdec->dec_err == 1) {
		fprintf(log, "\n\nDecoding error, unrepairable errors.\n");
	}

	fprintf(log, "\n\nSimplified decoding time: %.4f ms", (double)(timers->sdec_stop - timers->sdec_start) / 0xf4240);

	fprintf(log, "\n\n=====================Decoding=====================");
	fprintf(log, "\n\nSyndrome coefficients:\n");
	for (uint16_t i = 0; i < ERR_CORR_CAPABILITY * 2; i++) {
		if ((i > 1) && (i % (us->log_trim / 4) == 0))
			fprintf(log, "\n");
		fprintf(log, "%3d ", bch_dec->syndromes[i]);
	}

	fprintf(log, "\n\nError locator polynomial coefficients:\n");
	for (uint16_t i = 0; i <= bch_dec->l[ERR_CORR_CAPABILITY * 2 + 1]; i++) {
		if ((i > 1) && (i % (us->log_trim / 4) == 0))
			fprintf(log, "\n");
		fprintf(log, "%3d ", bch_dec->elp[ERR_CORR_CAPABILITY * 2 + 1][i]);
	}

	fprintf(log, "\n\nErrors positions:\n");
	for (uint8_t i = 0; i < us->err_amount; i++) {
		if ((i > 1) && (i % (us->log_trim / 5) == 0))
			fprintf(log, "\n");
		fprintf(log, "%d", bch_dec->loc[i]);
		if (i != us->err_amount - 1)
			fprintf(log, ", ");
	}

	fprintf(log, "\n\nDecoding time: %.4f ms", (double)(timers->dec_stop - timers->dec_start) / 0xf4240);

	fprintf(log, "\n\nExecution time: %.4f ms", (double)(timers->exec_stop - timers->exec_start) / 0xf4240);
}