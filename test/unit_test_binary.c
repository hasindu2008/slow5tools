#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h> // TODO need this?
#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"
#include "misc.h"

int slow5_open_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.blow5", "r");

    ASSERT(s5p != NULL);
    ASSERT(s5p->format == FORMAT_BINARY);
    ASSERT(s5p->header != NULL);
    ASSERT(s5p->header->version.major == 0);
    ASSERT(s5p->header->version.minor == 1);
    ASSERT(s5p->header->version.patch == 0);
    ASSERT(s5p->index == NULL);
    ASSERT(s5p->fp != NULL);

    ASSERT(slow5_hdr_print(s5p, FORMAT_ASCII) != -1);
    ASSERT(slow5_hdr_print(s5p, FORMAT_BINARY) != -1);

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
    ASSERT(slow5_hdr_set("asic_id_eeprom", "100", 0, s5p) == 0);
    ASSERT((mem = slow5_hdr_to_mem(s5p, FORMAT_BINARY, &len)) != NULL);
    ASSERT(fwrite(mem, len, 1, stdout) == 1);
    free(mem);

    char *str;
    ASSERT((str = slow5_hdr_to_mem(s5p, FORMAT_ASCII, &len)) != NULL);
    ASSERT(fwrite(str, len, 1, stdout) == 1);
    ASSERT(printf("%s", str) == len);
    free(str);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        CMD(slow5_open_valid)

        CMD(slow5_open_with_valid)
        CMD(slow5_open_with_invalid)

        CMD(slow5_hdr_to_mem_valid)
    };

    return RUN_TESTS(tests);
}
