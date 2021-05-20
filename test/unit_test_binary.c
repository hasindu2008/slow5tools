#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h> // TODO need this?
#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"
#include "slow5_idx.h"
#include "slow5_misc.h"

int slow5_idx_init_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_idx *idx = slow5_idx_init(s5p);
    ASSERT(idx != NULL);
    struct slow5_rec_idx read_idx;
    ASSERT(slow5_idx_get(idx, "a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read_idx) == 0);
    ASSERT(read_idx.offset == 1284);
    ASSERT(read_idx.size == 119442);

    slow5_idx_free(idx);
    ASSERT(slow5_close(s5p) == 0);


    s5p = slow5_open("test/data/exp/one_fast5/exp_1_default_gzip.blow5", "r");
    ASSERT(s5p != NULL);

    idx = slow5_idx_init(s5p);
    ASSERT(idx != NULL);
    ASSERT(slow5_idx_get(idx, "a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read_idx) == 0);
    ASSERT(read_idx.offset == 1284);

    slow5_idx_free(idx);
    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_idx_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_idx_create(s5p) == 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_idx_null(void) {
    ASSERT(slow5_idx_create(NULL) == -1);
    return EXIT_SUCCESS;
}

int slow5_idx_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/err/no_eof.blow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_idx_create(s5p) == -1);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_open_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");

    ASSERT(s5p != NULL);
    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->compress != NULL);
    ASSERT(s5p->compress->method == COMPRESS_NONE);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);

    ASSERT(slow5_hdr_print(s5p->header, FORMAT_ASCII, COMPRESS_NONE) != -1);
    ASSERT(slow5_hdr_print(s5p->header, FORMAT_BINARY, COMPRESS_NONE) != -1);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_open_with_valid(void) {
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.blow5", "r", FORMAT_BINARY);
    ASSERT(s5p != NULL);
    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);
    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open_with("test/data/err/one_fast5/invalid_extension.slow5", "r", FORMAT_BINARY);
    ASSERT(s5p != NULL);
    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);
    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.blow5", "r", FORMAT_UNKNOWN);
    ASSERT(s5p != NULL);
    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);
    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_open_with_invalid(void) {
    // Format invalid
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.blow5", "r", FORMAT_ASCII);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Extension invalid
    s5p = slow5_open_with("test/data/err/one_fast5/invalid_extension.blow", "r", FORMAT_UNKNOWN);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    return EXIT_SUCCESS;
}

int slow5_hdr_to_mem_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    void *mem;
    size_t len;
    ASSERT((mem = slow5_hdr_to_mem(s5p->header, FORMAT_BINARY, COMPRESS_NONE, &len)) != NULL);
    ASSERT(fwrite(mem, len, 1, stdout) == 1);
    free(mem);

    char *str;
    ASSERT((str = slow5_hdr_to_mem(s5p->header, FORMAT_ASCII, COMPRESS_NONE, &len)) != NULL);
    ASSERT(fwrite(str, len, 1, stdout) == 1);
    ASSERT(printf("%s", str) == len);
    free(str);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_to_mem_change_attr(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    void *mem;
    size_t len;
    ASSERT(slow5_hdr_set("asic_id_eeprom", "100", 0, s5p->header) == 0);
    ASSERT((mem = slow5_hdr_to_mem(s5p->header, FORMAT_BINARY, COMPRESS_NONE, &len)) != NULL);
    ASSERT(fwrite(mem, len, 1, stdout) == 1);
    free(mem);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_print_change_version(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    s5p->header->version.major = 10;
    s5p->header->version.minor = 200;

    ASSERT(slow5_hdr_print(s5p->header, FORMAT_BINARY, COMPRESS_NONE) > 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_to_mem_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(read != NULL);
    size_t size;
    void *mem = slow5_rec_to_mem(read, s5p->header->aux_meta, FORMAT_BINARY, NULL, &size);
    ASSERT(fwrite(mem, size, 1, stdout) == 1);
    free(mem);
    slow5_rec_free(read);
    ASSERT(slow5_eof_print() != -1);

    // TODO insert slow5 eof
    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_print_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(read != NULL);
    ASSERT(slow5_rec_print(read, s5p->header->aux_meta, FORMAT_BINARY, NULL) > 0);
    slow5_rec_free(read);
    ASSERT(slow5_eof_print() != -1);

    // TODO insert slow5 eof
    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_idx_load(s5p) == 0);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(strcmp(read->read_id, "a649a4ae-c43d-492a-b6a1-a5b8b8076be4") == 0);
    ASSERT(read->read_group == 0);
    ASSERT(read->digitisation == 8192);
    ASSERT(read->offset == 23);
    ASSERT(read->range == 1467.61);
    ASSERT(read->sampling_rate == 4000);
    ASSERT(read->len_raw_signal == 59676);
    ASSERT(read != NULL);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_invalid(void) {

    // No blow5 eof
    struct slow5_file *s5p = slow5_open("test/data/err/no_eof.blow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_idx_load(s5p) == -1);
    ASSERT(slow5_close(s5p) == 0);

    // Bad read id
    s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_idx_load(s5p) == 0);

     struct slow5_rec *read = NULL;
    ASSERT(slow5_get("badreadid", &read, s5p) == -3);
    ASSERT(slow5_get("", &read, s5p) == -3);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be", &read, s5p) == -3);
    ASSERT(slow5_get("O_O", &read, s5p) == -3);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    // TODO simulate -4 error

    // TODO simulate -5 error

    return EXIT_SUCCESS;
}

int slow5_open_gzip(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default_gzip.blow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->compress != NULL);
    ASSERT(s5p->compress->method == COMPRESS_GZIP);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_to_mem_gzip(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default_gzip.blow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_idx_load(s5p) == 0);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(read != NULL);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}


int main(void) {

    slow5_set_log_level(SLOW5_LOG_OFF);
    slow5_set_exit_condition(SLOW5_EXIT_OFF);

    struct command tests[] = {
        CMD(slow5_idx_init_valid)

        CMD(slow5_idx_valid)
        CMD(slow5_idx_null)
        CMD(slow5_idx_invalid)

        CMD(slow5_open_valid)

        CMD(slow5_open_with_valid)
        CMD(slow5_open_with_invalid)

        CMD(slow5_hdr_to_mem_valid)
        CMD(slow5_hdr_to_mem_change_attr)

        CMD(slow5_hdr_print_change_version)

        CMD(slow5_rec_to_mem_valid)

        CMD(slow5_rec_print_valid)

        CMD(slow5_get_valid)
        CMD(slow5_get_invalid)

        CMD(slow5_open_gzip)
        CMD(slow5_rec_to_mem_gzip)
    };

    return RUN_TESTS(tests);
}
