#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"

int slow5_init_empty_valid(void) {
    struct slow5_hdr *header = slow5_hdr_init_empty();
    slow5_hdr_free(header);

    return EXIT_SUCCESS;
}

int slow5_add(void) {
    struct slow5_hdr *header = slow5_hdr_init_empty();
    ASSERT(slow5_hdr_add_rg(header) == 0);
    ASSERT(slow5_hdr_add_rg(header) == 1);
    ASSERT(slow5_hdr_add_rg(header) == 2);
    ASSERT(slow5_hdr_add_attr("lol", header) == 0);
    ASSERT(slow5_hdr_add_attr("lol", header) == -2);
    ASSERT(slow5_hdr_print(header, FORMAT_ASCII, COMPRESS_NONE) != -1);
    slow5_hdr_free(header);

    return EXIT_SUCCESS;
}

int slow5_add_set(void) {
    struct slow5_hdr *header = slow5_hdr_init_empty();

    ASSERT(slow5_hdr_add_rg(header) == 0);
    ASSERT(slow5_hdr_add_rg(header) == 1);
    ASSERT(slow5_hdr_add_rg(header) == 2);
    ASSERT(slow5_hdr_add_attr("lol", header) == 0);
    ASSERT(slow5_hdr_add_attr("lol", header) == -2);
    ASSERT(slow5_hdr_set("lol", "haha", 2, header) == 0);
    ASSERT(slow5_hdr_set("lol", "good meme", 1, header) == 0);
    ASSERT(slow5_hdr_set("lol", "haha 2", 2, header) == 0);

    ASSERT(slow5_hdr_print(header, FORMAT_ASCII, COMPRESS_NONE) != -1);
    slow5_hdr_free(header);

    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        CMD(slow5_init_empty_valid)

        CMD(slow5_add)
        CMD(slow5_add_set)
    };

    return RUN_TESTS(tests);
}
