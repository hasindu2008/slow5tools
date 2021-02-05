#ifndef UNIT_TEST_H
#define UNIT_TEST

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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

int run_tests(struct command *tests, int num_tests) {
    int ret = EXIT_SUCCESS;
    int n_passed = 0;

    for (int i = 0; i < num_tests; ++ i) {
        if (tests[i].exe() == EXIT_FAILURE) {
            fprintf(stderr, "%s\t\t%s\n", tests[i].str, assert_fail);
            ret = EXIT_FAILURE;
        } else {
            ++ n_passed;
        }
    }

    // Print number of tests passed
    fprintf(stderr, "%d/%d\n", n_passed, num_tests);

    return ret;
}

#define RUN_TESTS(tests) run_tests(tests, sizeof tests / sizeof *tests);

#endif
