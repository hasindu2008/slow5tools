#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"

int slow5_hdr_init_empty_valid(void) {
    struct slow5_hdr *header = slow5_hdr_init_empty();
    slow5_hdr_free(header);

    return EXIT_SUCCESS;
}

int slow5_hdr_add(void) {
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

int slow5_hdr_add_set(void) {
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

int slow5_rec_init_empty_valid(void) {
    struct slow5_rec *read = slow5_rec_init();
    slow5_rec_free(read);

    return EXIT_SUCCESS;
}

/*
int slow5_rec_set_valid(void) {
    struct slow5_rec *read = slow5_rec_init();
    int32_t data = 3029;
    slow5_rec_add_ptr(read, &data, int32_t, 1);
    char *data = "lol";
    slow5_rec_add_str(read, data);

    return EXIT_SUCCESS;
}
*/


int main(void) {

    struct command tests[] = {
        CMD(slow5_hdr_init_empty_valid)

        CMD(slow5_hdr_add)
        CMD(slow5_hdr_add_set)

        CMD(slow5_rec_init_empty_valid)
        //CMD(slow5_rec_set_valid)
    };

    return RUN_TESTS(tests);
}
