#include <float.h>
#include "unit_test.h"
#include "slow5.h"

int slow5_open_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_rec_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    int err;
    char *cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    ASSERT(err == 0);
    double mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 225.114);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 222);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 1);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 2817564);
    ASSERT(err == 0);

    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open("test/data/exp/two_rg/exp_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    ASSERT(err == 0);
    cn = slow5_rec_get_str(read, "channel_number", NULL);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 225.114);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 222);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 1);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 2817564);
    ASSERT(err == 0);

    ASSERT(slow5_get("40aac17d-56a6-44db-934d-c0dbb853e2cd", &read, s5p) == 0);
    cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "123") == 0);
    ASSERT(err == 0);
    mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 220.292);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 2477);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 3);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 38528872);
    ASSERT(err == 0);

    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.blow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    ASSERT(err == 0);
    mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 225.114);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 222);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 1);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 2817564);
    ASSERT(err == 0);

    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless_gzip.blow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    ASSERT(err == 0);
    mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 225.114);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 222);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 1);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 2817564);
    ASSERT(err == 0);

    ASSERT(slow5_close(s5p) == 0);
    slow5_rec_free(read);

    return EXIT_SUCCESS;
}

int slow5_rec_get_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);

    int err;
    char *cn = slow5_rec_get_str(read, "channel_numbe", &err);
    ASSERT(cn == NULL);
    ASSERT(err == -1);
    double mb = slow5_rec_get_double(read, "", &err);
    ASSERT(mb == DBL_MAX);
    ASSERT(err == -1);
    ASSERT(slow5_rec_get_int32(read, "read_nu", &err) == INT32_MAX);
    ASSERT(err == -1);
    ASSERT(slow5_rec_get_uint8(read, "mux", &err) == UINT8_MAX);
    ASSERT(err == -1);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) != 2817563);
    ASSERT(err == 0);

    ASSERT(slow5_rec_get_str(read, "start_time", &err) == NULL);
    ASSERT(slow5_rec_get_str(NULL, "start_time", &err) == NULL);
    ASSERT(slow5_rec_get_str(NULL, NULL, &err) == NULL);
    ASSERT(slow5_rec_get_str(read, NULL, &err) == NULL);
    ASSERT(slow5_rec_get_str(read, NULL, NULL) == NULL);

    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int blow5_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.blow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int blow5_rec_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.blow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    int err;
    char *cn = slow5_rec_get_str(read, "channel_number", &err);
    ASSERT(cn != NULL);
    ASSERT(strcmp(cn, "115") == 0);
    ASSERT(err == 0);
    double mb = slow5_rec_get_double(read, "median_before", &err);
    ASSERT(mb == 225.114);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_int32(read, "read_number", &err) == 222);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint8(read, "start_mux", &err) == 1);
    ASSERT(err == 0);
    ASSERT(slow5_rec_get_uint64(read, "start_time", &err) == 2817564);
    ASSERT(err == 0);

    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_duplicate(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_hdr_print(s5p->header, FORMAT_ASCII, COMPRESS_NONE) != -1);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_rec_print(read, s5p->header->aux_meta, FORMAT_ASCII, NULL) != -1);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_to_blow5_uncomp(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_lossless.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_hdr_print(s5p->header, FORMAT_BINARY, COMPRESS_NONE) != -1);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_rec_print(read, s5p->header->aux_meta, FORMAT_BINARY, NULL) != -1);
    slow5_rec_free(read);

    slow5_eof_print();

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        CMD(slow5_open_valid)
        CMD(slow5_get_valid)
        CMD(slow5_rec_get_valid)
        CMD(slow5_rec_get_invalid)

        CMD(blow5_get_valid)
        CMD(blow5_rec_get_valid)

        CMD(slow5_duplicate)
        CMD(slow5_to_blow5_uncomp)
    };

    return RUN_TESTS(tests);
}
