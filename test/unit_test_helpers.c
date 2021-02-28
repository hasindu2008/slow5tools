#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h> // TODO need this?
#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"
#include "misc.h"

int ato_xintx_valid(void) {
    int err;
    char buf[256];

    ASSERT(ato_uint8("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint8("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint8("100", &err) == 100);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT8_MAX);
    ASSERT(ato_uint8(buf, &err) == UINT8_MAX);
    ASSERT(err == 0);

    ASSERT(ato_uint32("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint32("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint32("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_uint32("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT32_MAX);
    ASSERT(ato_uint32(buf, &err) == UINT32_MAX);
    ASSERT(err == 0);

    ASSERT(ato_uint64("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint64("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint64("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_uint64("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT32_MAX);
    ASSERT(ato_uint64(buf, &err) == UINT32_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%" PRId64, INT64_MAX);
    ASSERT(ato_uint64(buf, &err) == INT64_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu64, UINT64_MAX);
    ASSERT(ato_uint64(buf, &err) == UINT64_MAX);
    ASSERT(err == 0);

    sprintf(buf, "%" PRId16, (int16_t) INT16_MIN);
    ASSERT(ato_int16(buf, &err) == (int16_t) INT16_MIN);
    ASSERT(err == 0);
    ASSERT(ato_int16("-2000", &err) == -2000);
    ASSERT(err == 0);
    ASSERT(ato_int16("-100", &err) == -100);
    ASSERT(err == 0);
    ASSERT(ato_int16("-1", &err) == -1);
    ASSERT(err == 0);
    ASSERT(ato_int16("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_int16("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_int16("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_int16("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRId16, (int16_t) INT16_MAX);
    ASSERT(ato_int16(buf, &err) == (int16_t) INT16_MAX);
    ASSERT(err == 0);

    return EXIT_SUCCESS;
}

int ato_xintx_invalid(void) {
    int err;
    char buf[256];

    ASSERT(ato_uint8("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-1000", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-lol", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu16, (uint16_t) ( (uint16_t) UINT8_MAX + 1));
    ASSERT(ato_uint8(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu16, (uint16_t) UINT16_MAX);
    ASSERT(ato_uint8(buf, &err) == 0);
    ASSERT(err == -1);

    ASSERT(ato_uint32("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint32("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint32("-1000", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu64, (uint64_t) (UINT32_MAX) + 1);
    ASSERT(ato_uint32(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu64, UINT64_MAX);
    ASSERT(ato_uint32(buf, &err) == 0);
    ASSERT(err == -1);

    ASSERT(ato_uint64("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint64("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint64("-1000", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId64, INT64_MIN);
    ASSERT(ato_uint64(buf, &err) == 0);
    ASSERT(err == -1);

    sprintf(buf, "%" PRIu16, (uint16_t) UINT16_MAX);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId32, (int32_t) (INT16_MIN) - 1);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId32, (int32_t) (INT16_MAX) + 1);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId64, INT64_MIN);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);

    return EXIT_SUCCESS;
}

int strtod_check_valid(void) {
    int err;
    char buf[512];

    errno=0;

    ASSERT(strtod_check("0", &err) == (double) 0);
    ASSERT(err == 0);
    ASSERT(strtod_check("1", &err) == (double) 1);
    ASSERT(err == 0);
    ASSERT(strtod_check("100", &err) == (double) 100);
    ASSERT(err == 0);
    ASSERT(strtod_check("-100", &err) == (double) -100);
    ASSERT(err == 0);
    ASSERT(strtod_check("0.0", &err) == (double) 0.0);
    ASSERT(err == 0);
    ASSERT(strtod_check("2.5", &err) == (double) 2.5);
    ASSERT(err == 0);
    ASSERT(strtod_check("-100.7892", &err) == (double) -100.7892);
    ASSERT(err == 0);
    ASSERT(strtod_check("1.2", &err) == (double) 1.2);
    ASSERT(err == 0);
    sprintf(buf, "%lf", -DBL_MAX);
    ASSERT(strtod_check(buf, &err) == -DBL_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%lf", DBL_MIN);
    ASSERT(approx(strtod_check(buf, &err), DBL_MIN));
    ASSERT(err == 0);
    sprintf(buf, "%lf", DBL_MAX);
    ASSERT(strtod_check(buf, &err) == DBL_MAX);
    ASSERT(err == 0);

    return EXIT_SUCCESS;
}

int strtod_check_invalid(void) {
    int err;
    char buf[8192];

    ASSERT(strtod_check("hithere", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("sometext", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("inf", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("-inf", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%le", 10e3);
    ASSERT(strtod_check(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%le", -10e-3);
    ASSERT(strtod_check(buf, &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("-inf", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%Lf", LDBL_MAX);
    if (strcmp(buf, "inf") != 0) { // TODO why does valgrind give "inf"?
        ASSERT(strtod_check(buf, &err) == HUGE_VAL);
        ASSERT(err == -1);
    }
    sprintf(buf, "%Lf", -LDBL_MAX);
    if (strcmp(buf, "-inf") != 0) {
        ASSERT(strtod_check(buf, &err) == -HUGE_VAL);
        ASSERT(err == -1);
    }

    return EXIT_SUCCESS;
}

int has_fast5_ext_valid(void) {

    ASSERT(has_fast5_ext("test.fast5"));
    ASSERT(has_fast5_ext("hithere/test.fast5"));
    ASSERT(has_fast5_ext("testaskdj.fast5"));
    ASSERT(has_fast5_ext("fast5.fast5"));
    ASSERT(has_fast5_ext("slow5.fast5"));
    ASSERT(has_fast5_ext("hi...fast5.fast5"));
    ASSERT(has_fast5_ext("1234.fast5"));
    ASSERT(has_fast5_ext("myfast5.fast5"));
    ASSERT(has_fast5_ext("hithere///test.fast5"));
    ASSERT(has_fast5_ext("hithere///test.fast5/test.fast5"));
    ASSERT(has_fast5_ext("testaslkdjlaskjdfalsdifaslkfdj234.fast5"));
    ASSERT(has_fast5_ext(".fast5"));

    return EXIT_SUCCESS;
}

int has_fast5_ext_invalid(void) {

    ASSERT(!has_fast5_ext("."));
    ASSERT(!has_fast5_ext("..."));
    ASSERT(!has_fast5_ext("fast5"));
    ASSERT(!has_fast5_ext("fast5."));
    ASSERT(!has_fast5_ext("blow5"));
    ASSERT(!has_fast5_ext("blablabla"));
    ASSERT(!has_fast5_ext(""));
    ASSERT(!has_fast5_ext(NULL));

    return EXIT_SUCCESS;
}

int is_dir_valid(void) {

    ASSERT(is_dir("/"));
    ASSERT(is_dir("////////"));
    ASSERT(is_dir("."));
    ASSERT(is_dir("./"));
    ASSERT(is_dir(".."));
    ASSERT(is_dir("../"));
    ASSERT(is_dir("../test"));
    ASSERT(is_dir("../test/"));
    ASSERT(is_dir("data///"));
    ASSERT(is_dir("../src"));
    ASSERT(is_dir("random"));
    ASSERT(is_dir("data/exp/one_fast5"));

    return EXIT_SUCCESS;
}

int is_dir_invalid(void) {

    ASSERT(!is_dir("./unit_test"));
    ASSERT(!is_dir("notadir"));
    ASSERT(!is_dir("lolwhat"));
    ASSERT(!is_dir("///lolwhat///"));
    ASSERT(!is_dir("./unit_test.c"));
    ASSERT(!is_dir(""));
    ASSERT(!is_dir("../Make"));
    ASSERT(!is_dir("../Makefile"));
    ASSERT(!is_dir(NULL));

    return EXIT_SUCCESS;
}

int path_get_slow5_fmt_test(void) {

    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("test.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("hithere/test.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("testaskdj.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("blow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("slow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("hi...slow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("1234.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("myslow5.slow5"));

    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("blow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("slow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hi...blow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("1234.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("myblow5.blow5"));

    ASSERT(FORMAT_ASCII == path_get_slow5_fmt(".slow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt(".blow5"));

    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("..."));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("slow5"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("blow5"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("blablabla"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt(""));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt(NULL));

    return EXIT_SUCCESS;
}

int name_get_slow5_fmt_test(void) {

    ASSERT(FORMAT_ASCII == name_get_slow5_fmt("slow5"));
    ASSERT(FORMAT_BINARY == name_get_slow5_fmt("blow5"));

    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("..."));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow55"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow55"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blablabla"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(""));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(NULL));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("test.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere/test.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("testaskdj.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...slow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("myslow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...blow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("myblow5.blow5"));

    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        // Helpers
        CMD(ato_xintx_valid)
        CMD(ato_xintx_invalid)

        CMD(strtod_check_valid)
        CMD(strtod_check_invalid)

        CMD(has_fast5_ext_valid)
        CMD(has_fast5_ext_invalid)

        CMD(has_fast5_ext_valid)
        CMD(has_fast5_ext_invalid)

        CMD(path_get_slow5_fmt_test)
        CMD(name_get_slow5_fmt_test)
    };

    return RUN_TESTS(tests);
}
