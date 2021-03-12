#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"

int slow5_open_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/two_rg/exp_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(s5p->format == FORMAT_ASCII);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->header->num_read_groups == 2);
    ASSERT(s5p->header->data.num_attrs == 32);

    ASSERT(strcmp(slow5_hdr_get("asic_id", 1, s5p->header), "420170566") == 0);
    ASSERT(strcmp(slow5_hdr_get("experiment_type", 1, s5p->header), "") == 0);
    ASSERT(strcmp(slow5_hdr_get("file_version", 1, s5p->header), "0.6") == 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/two_rg/exp_default.slow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_idx_load(s5p) == 0);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("40aac17d-56a6-44db-934d-c0dbb853e2cd", &read, s5p) == 0);
    ASSERT(strcmp(read->read_id, "40aac17d-56a6-44db-934d-c0dbb853e2cd") == 0);
    ASSERT(read->read_group == 1);
    ASSERT(read->digitisation == 8192);
    ASSERT(read->offset == 26);
    ASSERT(read->range == 1444.86);
    ASSERT(read->sampling_rate == 4000);
    ASSERT(read->len_raw_signal == 46971);
    ASSERT(read->raw_signal[0] == 1233);
    ASSERT(read->raw_signal[read->len_raw_signal - 1] == 390);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_to_blow5_uncomp(void) {
    struct slow5_file *from = slow5_open("test/data/exp/two_rg/exp_default.slow5", "r");
    ASSERT(from != NULL);

    FILE *to = fopen("test/data/out/two_rg/out_default.blow5", "w");
    ASSERT(to != NULL);

    ASSERT(slow5_hdr_fwrite(to, from->header, FORMAT_BINARY, COMPRESS_NONE) != -1);

    struct slow5_rec *read = NULL;
    int ret;
    while ((ret = slow5_get_next(&read, from)) == 0) {
        ASSERT(slow5_rec_fwrite(to, read, from->header->aux_meta, FORMAT_BINARY, NULL) != -1);
    }
    slow5_rec_free(read);
    ASSERT(ret == -2);

    ASSERT(slow5_eof_fwrite(to) != -1);

    ASSERT(slow5_close(from) == 0);
    ASSERT(fclose(to) == 0);

    return EXIT_SUCCESS;
}

int slow5_to_blow5_gzip(void) {
    struct slow5_file *from = slow5_open("test/data/exp/two_rg/exp_default.slow5", "r");
    ASSERT(from != NULL);

    FILE *to = fopen("test/data/out/two_rg/out_default_gzip.blow5", "w");
    ASSERT(to != NULL);

    ASSERT(slow5_hdr_fwrite(to, from->header, FORMAT_BINARY, COMPRESS_GZIP) != -1);

    struct slow5_rec *read = NULL;
    int ret;
    struct press *gzip = press_init(COMPRESS_GZIP);
    ASSERT(gzip != NULL);
    while ((ret = slow5_get_next(&read, from)) == 0) {
        ASSERT(slow5_rec_fwrite(to, read, from->header->aux_meta, FORMAT_BINARY, gzip) != -1);
    }
    slow5_rec_free(read);
    ASSERT(ret == -2);

    ASSERT(slow5_eof_fwrite(to) != -1);

    press_free(gzip);
    ASSERT(slow5_close(from) == 0);
    ASSERT(fclose(to) == 0);

    return EXIT_SUCCESS;
}

int slow5_add_rg_data_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_file *s5p_two = slow5_open("test/data/exp/two_rg/exp_default.slow5", "r");
    ASSERT(s5p_two != NULL);

    khash_t(s2s) *rg_two = slow5_hdr_get_data(1, s5p_two->header);
    ASSERT(rg_two != NULL);
    ASSERT(slow5_hdr_add_rg_data(s5p->header, rg_two) == 1);

    ASSERT(s5p->header->num_read_groups == 2);

    slow5_hdr_print(s5p->header, FORMAT_ASCII, COMPRESS_NONE);

    ASSERT(slow5_close(s5p) == 0);
    ASSERT(slow5_close(s5p_two) == 0);
    return EXIT_SUCCESS;
}

int main(void) {

    struct command tests[] = {
        CMD(slow5_open_valid)

        CMD(slow5_get_valid)

        CMD(slow5_to_blow5_uncomp)
        CMD(slow5_to_blow5_gzip)

        CMD(slow5_add_rg_data_valid)
    };

    return RUN_TESTS(tests);
}
