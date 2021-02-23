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


int slow5_rec_set_valid(void) {
    /*
    struct slow5_rec *read = slow5_rec_init();

    struct slow5_aux_meta *aux_meta = slow5_aux_meta_init_empty();
    ASSERT(SLOW5_AUX_META_ADD(aux_meta, "channel_number", char*) == 0);
    ASSERT(slow5_aux_meta_add(aux_meta, "median_before", "double") == 0);
    ASSERT(SLOW5_AUX_META_ADD(aux_meta, "read_number", int32_t) == 0);
    ASSERT(slow5_aux_meta_add(aux_meta, "start_mux", "uint8_t") == 0);
    ASSERT(slow5_aux_meta_add(aux_meta, "start_time", "uint64_t") == 0);

    char *cn = "1010";
    double mb = 225.69;
    int32_t rn = 292;
    uint8_t sm = 1;
    uint64_t st = 1019283;

    ASSERT(SLOW5_REC_ADD_ARRAY(read, aux_meta, "channel_number", char*, &cn, strlen(cn) + 1) == 0);
    //ASSERT(slow5_rec_add_array(read, "channel_number", "char*", &cn, strlen(cn) + 1) == 0);
    //ASSERT(slow5_rec_add_str(read, "channel_number", cn) == 0);
    ASSERT(slow5_rec_add(read, aux_meta, "start_time", "uint64_t", &st) == 0);
    ASSERT(slow5_rec_add(read, aux_meta, "read_number", "int32_t", &rn) == 0);
    ASSERT(slow5_rec_add(read, aux_meta, "median_before", "double", &mb) == 0);
    ASSERT(slow5_rec_add(read, aux_meta, "start_mux", "uint8_t", &sm) == 0);

    ASSERT(slow5_rec_print(read, FORMAT_ASCII, COMPRESS_NONE) != -1);
    */

    return EXIT_SUCCESS;
}


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
