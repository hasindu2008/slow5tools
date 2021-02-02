#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h> // TODO need this?
#include "slow5.h"
#include "slow5_extra.h"
#include "misc.h"

#define ASSERT(statement) \
if (!(statement)) { \
    sprintf(assert_fail, "line %d: assertion `%s' failed", __LINE__, #statement); \
    return EXIT_FAILURE; \
}

#define CMD(foo) {#foo, foo},

#define APPROX_MARGIN (1e-100)

static char assert_fail[256];

typedef struct command command_t;

struct command {
	const char *str;
	int (*exe)(void);
};

bool approx(double x, double y) {
    return ((x - y) <= APPROX_MARGIN &&
            (x - y) >= -APPROX_MARGIN);
}

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

    sprintf(buf, "%" PRId16, INT16_MIN);
    ASSERT(ato_int16(buf, &err) == INT16_MIN);
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
    sprintf(buf, "%" PRId16, INT16_MAX);
    ASSERT(ato_int16(buf, &err) == INT16_MAX);
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
    sprintf(buf, "%" PRIu16, (uint16_t) (UINT8_MAX) + 1);
    ASSERT(ato_uint8(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu16, UINT16_MAX);
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

    sprintf(buf, "%" PRIu16, UINT16_MAX);
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

    // TODO should this be true
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

int slow5_open_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);
    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_open_null(void) {
    // Pathname NULL
    struct slow5_file *s5p = slow5_open(NULL, "r");
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Mode NULL
    s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", NULL);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Both NULL
    s5p = slow5_open(NULL, NULL);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    return EXIT_SUCCESS;
}

int slow5_open_invalid(void) {
    // Path invalid
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5_invalid/exp_1_default.slow5", "r");
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Extension invalid
    s5p = slow5_open("test/data/err/one_fast5/invalid_extension.slow", "r");
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    return EXIT_SUCCESS;
}

int slow5_open_with_valid(void) {
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.slow5", "r", FORMAT_ASCII);
    ASSERT(s5p != NULL);
    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open_with("test/data/err/one_fast5/invalid_extension.slow", "r", FORMAT_ASCII);
    ASSERT(s5p != NULL);
    ASSERT(slow5_close(s5p) == 0);

    s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.slow5", "r", FORMAT_UNKNOWN);
    ASSERT(s5p != NULL);
    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_open_with_null(void) {
    // Pathname NULL
    struct slow5_file *s5p = slow5_open_with(NULL, "r", FORMAT_ASCII);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Mode NULL
    s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.slow5", NULL, FORMAT_BINARY);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Both NULL
    s5p = slow5_open_with(NULL, NULL, FORMAT_UNKNOWN);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    return EXIT_SUCCESS;
}

int slow5_open_with_invalid(void) {
    // Format invalid
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1_default.slow5", "r", FORMAT_BINARY);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    // Extension invalid
    s5p = slow5_open_with("test/data/err/one_fast5/invalid_extension.slow", "r", FORMAT_UNKNOWN);
    ASSERT(s5p == NULL);
    ASSERT(slow5_close(s5p) == EOF);

    return EXIT_SUCCESS;
}

int slow5_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get(NULL, &read, s5p) == -1);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", NULL, s5p) == -1);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, NULL) == -1);
    ASSERT(slow5_get(NULL, NULL, s5p) == -1);
    ASSERT(slow5_get(NULL, &read, NULL) == -1);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", NULL, NULL) == -1);
    ASSERT(slow5_get(NULL, NULL, NULL) == -1);
    // Shouldn't need to slow5_rec_free

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    // TODO simulate -2 error

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("badreadid", &read, s5p) == -3);
    ASSERT(slow5_get("", &read, s5p) == -3);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be", &read, s5p) == -3);
    ASSERT(slow5_get("O_O", &read, s5p) == -3);

    ASSERT(slow5_close(s5p) == 0);

    // TODO simulate -4 error

    s5p = slow5_open("test/data/err/parse_bad.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == -5);
    ASSERT(slow5_get("1", &read, s5p) == -5);
    ASSERT(slow5_get("2", &read, s5p) == -5);
    ASSERT(slow5_get("3", &read, s5p) == -5);
    ASSERT(slow5_get("4", &read, s5p) == -5);
    ASSERT(slow5_get("5", &read, s5p) == -5);
    ASSERT(slow5_get("6", &read, s5p) == -5);
    ASSERT(slow5_get("7", &read, s5p) == -5);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_many_same(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    ASSERT(slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_many_different(void) {
    struct slow5_file *s5p = slow5_open("test/data/test/same_diff_ids.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get("1", &read, s5p) == 0);
    ASSERT(slow5_get("2", &read, s5p) == 0);
    ASSERT(slow5_get("3", &read, s5p) == 0);
    ASSERT(slow5_get("4", &read, s5p) == 0);
    ASSERT(slow5_get("5", &read, s5p) == 0);
    ASSERT(slow5_get("6", &read, s5p) == 0);
    ASSERT(slow5_get("7", &read, s5p) == 0);
    ASSERT(slow5_get("8", &read, s5p) == 0);
    ASSERT(slow5_get("9", &read, s5p) == 0);
    ASSERT(slow5_get("10", &read, s5p) == 0);
    ASSERT(slow5_get("--", &read, s5p) == 0);
    ASSERT(slow5_get("", &read, s5p) == 0);
    ASSERT(slow5_get(".", &read, s5p) == 0);
    ASSERT(slow5_get("^_^", &read, s5p) == 0);
    ASSERT(slow5_get("lol", &read, s5p) == 0);
    ASSERT(slow5_get("weirdid", &read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}


int slow5_get_next_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_next_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(NULL, s5p) == -1);
    ASSERT(slow5_get_next(&read, NULL) == -1);
    ASSERT(slow5_get_next(NULL, NULL) == -1);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_next_empty(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_get_next(&read, s5p) == -2);
    ASSERT(slow5_get_next(&read, s5p) == -2);
    ASSERT(slow5_get_next(&read, s5p) == -2);
    ASSERT(slow5_get_next(&read, s5p) == -2);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);

    return EXIT_SUCCESS;
}

int slow5_get_next_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/err/parse_bad.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    ASSERT(slow5_get_next(&read, s5p) == -3);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_to_str_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    char *str = slow5_rec_to_str(read, FORMAT_ASCII);
    printf("%s\n", str);
    free(str);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_to_str_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_rec_to_str(NULL, FORMAT_ASCII) == NULL);
    ASSERT(slow5_rec_to_str(read, FORMAT_UNKNOWN) == NULL);
    ASSERT(slow5_rec_to_str(NULL, FORMAT_UNKNOWN) == NULL);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_to_str_change(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    free(read->read_id);
    read->read_id = strdup("testing123");
    char *str;
    ASSERT((str = slow5_rec_to_str(read, FORMAT_ASCII)) != NULL);
    printf("%s\n", str);
    free(str);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_print_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_rec_print(read, FORMAT_ASCII) == 238771);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_print_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);

    ASSERT(slow5_rec_print(NULL, FORMAT_ASCII) == -1);
    ASSERT(slow5_rec_print(read, FORMAT_UNKNOWN) == -1);
    ASSERT(slow5_rec_print(NULL, FORMAT_UNKNOWN) == -1);

    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_print_change(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    free(read->read_id);
    read->read_id = strdup("lol");
    ASSERT(slow5_rec_print(read, FORMAT_ASCII) == 238738);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_fprint_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    FILE *fp;
    ASSERT((fp = fopen("test/data/out/unit_test_out_fprint", "w")) != NULL);
    ASSERT(slow5_rec_fprint(fp, read, FORMAT_ASCII) == 238771);
    slow5_rec_free(read);

    ASSERT(fclose(fp) == 0);
    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_fprint_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    ASSERT(slow5_rec_fprint(NULL, read, FORMAT_ASCII) == -1);
    ASSERT(slow5_rec_fprint(stdout, NULL, FORMAT_ASCII) == -1);
    ASSERT(slow5_rec_fprint(NULL, NULL, FORMAT_ASCII) == -1);
    slow5_rec_free(read);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_rec_fprint_change(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    struct slow5_rec *read = NULL;
    ASSERT(slow5_get_next(&read, s5p) == 0);
    free(read->read_id);
    read->read_id = strdup("lol");
    FILE *fp;
    ASSERT((fp = fopen("test/data/out/unit_test_out_fprint", "a")) != NULL);
    ASSERT(slow5_rec_fprint(fp, read, FORMAT_ASCII) == 238738);
    slow5_rec_free(read);

    ASSERT(fclose(fp) == 0);
    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_get_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "3574887596") == 0);
    ASSERT(strcmp(slow5_hdr_get("asic_id_eeprom", 0, s5p), "0") == 0);
    ASSERT(strcmp(slow5_hdr_get("asic_temp", 0, s5p), "29.2145729") == 0);
    ASSERT(strcmp(slow5_hdr_get("auto_update_source", 0, s5p), "https://mirror.oxfordnanoportal.com/software/MinKNOW/") == 0);
    ASSERT(strcmp(slow5_hdr_get("bream_core_version", 0, s5p), "1.1.20.1") == 0);
    ASSERT(strcmp(slow5_hdr_get("usb_config", 0, s5p), "1.0.11_ONT#MinION_fpga_1.0.1#ctrl#Auto") == 0);
    ASSERT(strcmp(slow5_hdr_get("version", 0, s5p), "1.1.20") == 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_get_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_hdr_get(NULL, 0, s5p) == NULL);
    ASSERT(slow5_hdr_get("asic_id", -1, s5p) == NULL);
    ASSERT(slow5_hdr_get("asic_id", 1, s5p) == NULL);
    ASSERT(slow5_hdr_get("asic_id", 20, s5p) == NULL);
    ASSERT(slow5_hdr_get("asic_id", UINT32_MAX, s5p) == NULL);
    ASSERT(slow5_hdr_get("asic_id", 0, NULL) == NULL);
    ASSERT(slow5_hdr_get(NULL, 0, NULL) == NULL);
    ASSERT(slow5_hdr_get(NULL, -1, s5p) == NULL);
    ASSERT(slow5_hdr_get("auto_update_source", -1, NULL) == NULL);
    ASSERT(slow5_hdr_get(NULL, -200, NULL) == NULL);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_get_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "3574887597") != 0);
    ASSERT(strcmp(slow5_hdr_get("asic_id_eeprom", 0, s5p), "100.0") != 0);
    ASSERT(strcmp(slow5_hdr_get("asic_temp", 0, s5p), "292145729") != 0);
    ASSERT(strcmp(slow5_hdr_get("auto_update_source", 0, s5p), "ttps://mirror.oxfordnanoportal.com/software/MinKNOW/") != 0);
    ASSERT(strcmp(slow5_hdr_get("bream_core_version", 0, s5p), "1..20.1") != 0);
    ASSERT(strcmp(slow5_hdr_get("usb_config", 0, s5p), "1.0.11_ONTMinION_fpga_1.0.1#ctrl#Auto") != 0);
    ASSERT(strcmp(slow5_hdr_get("version", 0, s5p), "1.1.2") != 0);

    ASSERT(slow5_hdr_get("file_format", 0, s5p) == NULL);
    ASSERT(slow5_hdr_get("num_read_groups", 0, s5p) == NULL);
    ASSERT(slow5_hdr_get("lol", 0, s5p) == NULL);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_get_set(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    char *asic_id = slow5_hdr_get("asic_id", 0, s5p);
    asic_id[0] = '!';
    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "!574887596") == 0);
    asic_id = "new ptr";
    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "!574887596") == 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_set_valid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_hdr_set("asic_id", "<new asic id>", 0, s5p) == 0);
    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "3574887596") != 0);
    ASSERT(strcmp(slow5_hdr_get("asic_id", 0, s5p), "<new asic id>") == 0);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_set_null(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    ASSERT(slow5_hdr_set("asic_id", "<new asic id>", 0, NULL) == -1);
    ASSERT(slow5_hdr_set("asic_id", "<new asic id>", -1, s5p) == -1);
    ASSERT(slow5_hdr_set("asic_id", NULL, 0, s5p) == -1);
    ASSERT(slow5_hdr_set(NULL, "<new asic id>", 0, s5p) == -1);
    ASSERT(slow5_hdr_set(NULL, NULL, 0, s5p) == -1);
    ASSERT(slow5_hdr_set(NULL, "<new asic id>", -1, s5p) == -1);
    ASSERT(slow5_hdr_set(NULL, "<new asic id>", 0, NULL) == -1);
    ASSERT(slow5_hdr_set(NULL, NULL, 0, NULL) == -1);
    ASSERT(slow5_hdr_set(NULL, NULL, -1, s5p) == -1);
    ASSERT(slow5_hdr_set(NULL, "<new asic id>", -1, NULL) == -1);
    ASSERT(slow5_hdr_set("asic_id", NULL, -1, NULL) == -1);
    ASSERT(slow5_hdr_set(NULL, NULL, -1, NULL) == -1);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_set_invalid(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    // Wrong attribute name
    ASSERT(slow5_hdr_set("file_format", "x", 0, s5p) == -1);
    ASSERT(slow5_hdr_set("num_read_groups", "x", 0, s5p) == -1);
    ASSERT(slow5_hdr_set("lol", "x", 0, s5p) == -1);

    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}

int slow5_hdr_set_malloc(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1_default.slow5", "r");
    ASSERT(s5p != NULL);

    char *new_version = malloc(10 * sizeof *new_version);
    new_version[0] = '1';
    new_version[1] = '.';
    new_version[2] = '2';
    new_version[3] = '\0';

    ASSERT(slow5_hdr_set("version", new_version, 0, s5p) == 0);
    ASSERT(strcmp(slow5_hdr_get("version", 0, s5p), new_version) == 0);

    free(new_version);
    ASSERT(slow5_close(s5p) == 0);
    return EXIT_SUCCESS;
}


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

    // API
    CMD(slow5_open_valid)
    CMD(slow5_open_null)
    CMD(slow5_open_invalid)

    CMD(slow5_open_with_valid)
    CMD(slow5_open_with_null)
    //CMD(slow5_open_with_invalid) // TODO blow5 reading

    CMD(slow5_get_valid)
    CMD(slow5_get_null)
    CMD(slow5_get_invalid)
    CMD(slow5_get_many_same)
    CMD(slow5_get_many_different)

    CMD(slow5_get_next_valid)
    CMD(slow5_get_next_null)
    CMD(slow5_get_next_empty)
    CMD(slow5_get_next_invalid)

    CMD(slow5_rec_to_str_valid)
    CMD(slow5_rec_to_str_null)
    CMD(slow5_rec_to_str_change)

    CMD(slow5_rec_print_valid)
    CMD(slow5_rec_print_null)
    CMD(slow5_rec_print_change)

    CMD(slow5_rec_fprint_valid)
    CMD(slow5_rec_fprint_null)
    CMD(slow5_rec_fprint_change)

    CMD(slow5_hdr_get_valid)
    CMD(slow5_hdr_get_null)
    CMD(slow5_hdr_get_invalid)
    CMD(slow5_hdr_get_set)

    CMD(slow5_hdr_set_valid)
    CMD(slow5_hdr_set_null)
    CMD(slow5_hdr_set_invalid)
    CMD(slow5_hdr_set_malloc)
};

int main(void) {
    int test_n = sizeof (tests) / sizeof (struct command);
    int ret = EXIT_SUCCESS;
    int n_passed = 0;

    for (int i = 0; i < test_n; ++ i) {
        if (tests[i].exe() == EXIT_FAILURE) {
            fprintf(stderr, "%s\t\t%s\n", tests[i].str, assert_fail);
            ret = EXIT_FAILURE;
        } else {
            ++ n_passed;
        }
    }

    // Print number of tests passed
    fprintf(stderr, "%d/%d\n", n_passed, test_n);

    return ret;
}
