#include <assert.h>
#include <stdio.h>
#include "slow5.h"
#include "slow5_extra.h"

#define CMD(foo) {#foo, foo},

typedef struct command command_t;

struct command {
	const char *str;
	int (*exe)(void);
};

int test_path_get_slow5_fmt(void) {

    assert(FORMAT_ASCII == path_get_slow5_fmt("test.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("hithere/test.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("testaskdj.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("blow5.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("slow5.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("hi...slow5.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("1234.slow5"));
    assert(FORMAT_ASCII == path_get_slow5_fmt("myslow5.slow5"));

    assert(FORMAT_BINARY == path_get_slow5_fmt("test.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("blow5.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("slow5.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("hi...blow5.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("1234.blow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt("myblow5.blow5"));

    // TODO should this be true
    assert(FORMAT_ASCII == path_get_slow5_fmt(".slow5"));
    assert(FORMAT_BINARY == path_get_slow5_fmt(".blow5"));

    assert(FORMAT_UNKNOWN == path_get_slow5_fmt("..."));
    assert(FORMAT_UNKNOWN == path_get_slow5_fmt("slow5"));
    assert(FORMAT_UNKNOWN == path_get_slow5_fmt("blow5"));
    assert(FORMAT_UNKNOWN == path_get_slow5_fmt("blablabla"));
    assert(FORMAT_UNKNOWN == path_get_slow5_fmt(""));
    assert(FORMAT_UNKNOWN == path_get_slow5_fmt(NULL));

    return 1;
}

int test_name_get_slow5_fmt(void) {

    assert(FORMAT_ASCII == name_get_slow5_fmt("slow5"));
    assert(FORMAT_BINARY == name_get_slow5_fmt("blow5"));

    assert(FORMAT_UNKNOWN == name_get_slow5_fmt(".slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("..."));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("slow55"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("blow55"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("blablabla"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt(""));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt(NULL));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("test.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere/test.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("testaskdj.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...slow5.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("myslow5.slow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("test.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...blow5.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.blow5"));
    assert(FORMAT_UNKNOWN == name_get_slow5_fmt("myblow5.blow5"));

    return 1;
}

int open_close_slow5_file(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);
    assert(slow5_close(s5p) == 0);

    return 1;
}

int open_with_close_slow5_file(void) {
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1.slow5", "r", FORMAT_ASCII);
    assert(s5p != NULL);
    assert(slow5_close(s5p) == 0);

    return 1;
}

/*
int open_with_close_slow5_file_fail(void) {
    struct slow5_file *s5p = slow5_open_with("test/data/exp/one_fast5/exp_1.slow5", "r", FORMAT_BINARY);
    assert(s5p == NULL);
    assert(slow5_close(s5p) == EOF);

    return 1;
}
*/

int get_read(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}

int get_read_and_print(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_rec_print(read);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}

int get_reads(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}

int get_next_read(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get_next(&read, s5p);
    slow5_rec_print(read);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}

/*
int get_next_read_empty(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get_next(&read, s5p);
    slow5_rec_print(read);
    slow5_get_next(&read, s5p);
    slow5_rec_print(read);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}
*/

int get_many_same_reads(void) {
    struct slow5_file *s5p = slow5_open("test/data/exp/one_fast5/exp_1.slow5", "r");
    assert(s5p != NULL);

    struct slow5_rec *read = NULL;
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_get("a649a4ae-c43d-492a-b6a1-a5b8b8076be4", &read, s5p);
    slow5_rec_free(read);

    assert(slow5_close(s5p) == 0);

    return 1;
}

struct command tests[] = {
    CMD(test_path_get_slow5_fmt)
    CMD(test_name_get_slow5_fmt)
    CMD(open_close_slow5_file)
    CMD(open_with_close_slow5_file)
    //CMD(open_with_close_slow5_file_fail)
    CMD(get_read)
    CMD(get_read_and_print)
    CMD(get_reads)
    CMD(get_many_same_reads)
    CMD(get_next_read)
};

int main(void) {
    int test_n = sizeof (tests) / sizeof (struct command);
    int ret = 0;

    for (int i = 0; i < test_n; ++ i) {
        if (tests[i].exe()) {
            fprintf(stderr, "%s Passed\n", tests[i].str);
        } else {
            fprintf(stderr, "%s Failed\n", tests[i].str);
            ret = 1;
        }
    }

    return ret;
}
