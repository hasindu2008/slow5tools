#include "unit_test.h"
#include "press.h"
#include <string.h>

int press_init_valid(void) {
    struct press *comp = press_init(COMPRESS_NONE);
    ASSERT(comp->method == COMPRESS_NONE);
    ASSERT(comp->stream == NULL);
    press_free(comp);

    comp = press_init(COMPRESS_GZIP);
    ASSERT(comp->method == COMPRESS_GZIP);
    ASSERT(comp->stream != NULL);
    press_free(comp);

    return EXIT_SUCCESS;
}

int press_buf_valid(void) {
    struct press *comp = press_init(COMPRESS_NONE);

    const char *str = "hello";
    size_t size = 0;
    char *str_same = str_compress(comp, str, &size);
    ASSERT(strcmp(str_same, str) == 0);
    ASSERT(size == strlen(str) + 1);

    press_free(comp);

    comp = press_init(COMPRESS_GZIP);
    size_t size_gzip = 0;
    compress_footer_next(comp);
    void *str_gzip = str_compress(comp, str, &size_gzip);
    char *str_copy = ptr_depress(comp, str_gzip, size_gzip, &size);
    ASSERT(strcmp(str_copy, str) == 0);
    ASSERT(size == strlen(str_copy) + 1);
    ASSERT(size == strlen(str_same) + 1);
    ASSERT(size == strlen(str) + 1);
    fwrite(str_gzip, size_gzip, 1, stdout); // TESTING

    free(str_gzip);
    free(str_same);
    free(str_copy);
    press_free(comp);

    return EXIT_SUCCESS;
}

int press_print_valid(void) {
    struct press *comp = press_init(COMPRESS_GZIP);

    const char *str = "hello";
    compress_footer_next(comp);
    print_str_compress(comp, str);

    press_free(comp);

    return EXIT_SUCCESS;
}

int press_printf_valid(void) {
    struct press *comp = press_init(COMPRESS_GZIP);

    const char *str = "lol";
    compress_footer_next(comp);
    printf_compress(comp, "\n%s\n", str);

    press_free(comp);

    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        CMD(press_init_valid)

        CMD(press_buf_valid)

        CMD(press_print_valid)

        CMD(press_printf_valid)
    };

    return RUN_TESTS(tests);
}
