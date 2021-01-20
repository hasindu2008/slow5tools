#include <assert.h>
#include <stdio.h>
#include "slow5.h"
#include "slow5tools.h"

#define CMD(foo) {#foo, foo},

typedef struct command command_t;

typedef struct command {
	char* str;
	int (*exe)(void);
};

int test_pathname_get_slow5_format(void) {

    assert(FORMAT_ASCII == pathname_get_slow5_format("test.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("hithere/test.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("testaskdj.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("blow5.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("slow5.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("hi...slow5.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("1234.slow5"));
    assert(FORMAT_ASCII == pathname_get_slow5_format("myslow5.slow5"));

    assert(FORMAT_BINARY == pathname_get_slow5_format("test.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("hithere///test.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("hithere///test.slow5/test.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("blow5.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("slow5.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("hi...blow5.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("1234.blow5"));
    assert(FORMAT_BINARY == pathname_get_slow5_format("myblow5.blow5"));

    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("..."));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("slow5");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blow5");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blablabla");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(NULL);

    return 1;
}

int test_str_get_slow5_format(void) {

    assert(FORMAT_ASCII == str_get_slow5_format("slow5"));
    assert(FORMAT_BINARY == str_get_slow5_format("blow5"));

    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(".blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("..."));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("slow5");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blow5");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blablabla");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("");
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format(NULL);
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("test.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("hithere/test.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("testaskdj.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blow5.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("slow5.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("hi...slow5.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("1234.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("myslow5.slow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("test.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("hithere///test.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("hithere///test.slow5/test.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("blow5.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("slow5.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("hi...blow5.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("1234.blow5"));
    assert(FORMAT_UNKNOWN == pathname_get_slow5_format("myblow5.blow5"));

    return 1;
}

command_t tests[] = {
    CMD(test_pathname_get_slow5_format)
    CMD(test_str_get_slow5_format)
};

int main(void) {
    int test_n = sizeof (tests) / sizeof (command_t);

    for (int i = 0; i < test_n; ++ i) {
        if (tests[i].exe()) {
            fprintf(stdout, "%s Passed\n", tests[i].str);
        } else {
            fprintf(stdout, "%s Failed\n", tests[i].str);
        }
    }

    return 0;
}

