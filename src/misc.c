// Miscellaneous helper functions

#include <zlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include "misc.h"
#include "error.h"
#include "fast5.h"


// FAST5

/* Check if path has the fast5 extension
 *
 * @param       file path
 * @return      whether path has the fast5 extension
 */
bool has_fast5_ext(const char *f_path) {
    bool ret = false;

    if (f_path != NULL) {
        size_t f_path_len = strlen(f_path);
        size_t fast5_ext_len = strlen(FAST5_EXTENSION);

        if (f_path_len >= fast5_ext_len &&
                strcmp(f_path + (f_path_len - fast5_ext_len), FAST5_EXTENSION) == 0) {
            ret = true;
        }
    }

    return ret;
}

// From https://stackoverflow.com/questions/26522583/c-strtok-skips-second-token-or-consecutive-delimiter
char *strtok_solo(char *str, char *seps) {
    static char *tpos, *tkn, *pos = NULL;
    static char savech;

    // Specific actions for first and subsequent calls.

    if (str != NULL) {
        // First call, set pointer.

        pos = str;
        savech = 'x';
    } else {
        // Subsequent calls, check we've done first.

        if (pos == NULL)
            return NULL;

        // Then put character back and advance.

        while (*pos != '\0')
            pos++;
        *pos++ = savech;
    }

    // Detect previous end of string.

    if (savech == '\0')
        return NULL;

    // Now we have pos pointing to first character.
    // Find first separator or nul.

    tpos = pos;
    while (*tpos != '\0') {
        tkn = strchr (seps, *tpos);
        if (tkn != NULL)
            break;
        tpos++;
    }

    savech = *tpos;
    *tpos = '\0';

    return pos;
}

// Atoi but to uint64_t
// and without any symbols
// and without 0 prefixing
uint64_t ato_uint64(const char *str) {
    uint64_t ret = 0;

    // Ensure first number is not 0 if more letters in string
    if (strlen(str) > 1) {
        assert(str[0] != '0');
    }
    // Ensure only integers in string
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert(str[i] >= 48 && str[i] <= 57);
    }

    ret = strtoull(str, NULL, 10);
    assert(ret != ULLONG_MAX && errno != ERANGE);

    return ret;
}

// Atoi but to uint32_t
// and without any symbols
// and without 0 prefixing
uint32_t ato_uint32(const char *str) {
    uint32_t ret = 0;

    // Ensure first number is not 0 if more letters in string
    if (strlen(str) > 1) {
        assert(str[0] != '0');
    }
    // Ensure only integers in string
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert(str[i] >= 48 && str[i] <= 57);
    }

    ret = strtoul(str, NULL, 10);
    assert(ret != ULONG_MAX && errno != ERANGE);

    return ret;
}

// Atoi but to uint8_t
// and without any symbols
// and without 0 prefixing
uint8_t ato_uint8(const char *str) {
    uint8_t ret = 0;

    // Ensure first number is not 0 if more letters in string
    if (strlen(str) > 1) {
        assert(str[0] != '0');
    }
    // Ensure only integers in string
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert(str[i] >= 48 && str[i] <= 57);
    }

    unsigned long int tmp = strtoul(str, NULL, 10);
    assert(tmp <= UINT8_MAX);
    ret = (uint8_t) tmp;

    return ret;
}

// Atoi but to int16_t
// and without any symbols
// and without 0 prefixing
int16_t ato_int16(const char *str) {
    uint8_t ret = 0;

    // Ensure first number is not 0 if more letters in string
    if (strlen(str) > 1) {
        assert(str[0] != '0');
    }
    // Ensure only integers in string
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert(str[i] >= 48 && str[i] <= 57);
    }

    long int tmp = strtol(str, NULL, 10);
    assert(tmp <= INT16_MAX && tmp >= INT16_MIN);
    ret = (int16_t) tmp;

    return ret;
}

double strtod_check(const char *str) {

    // Ensure only integers in string or '.'
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert((str[i] >= 48 && str[i] <= 57) || str[i] == '.');
    }

    double ret = strtod(str, NULL);
    if (errno == ERANGE) {
        assert(ret != HUGE_VAL && ret != -HUGE_VAL && ret != 0);
    }

    return ret;
}

float strtof_check(const char *str) {

    // Ensure only integers in string or '.'
    for (size_t i = 0; i < strlen(str); ++ i) {
        assert((str[i] >= 48 && str[i] <= 57) || str[i] == '.');
    }

    double ret = strtof(str, NULL);
    if (errno == ERANGE) {
        assert(ret != HUGE_VAL && ret != -HUGE_VAL && ret != 0);
    }

    return ret;
}
