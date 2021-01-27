// Miscellaneous helper functions

#include <zlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
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

// From https://code.woboq.org/userspace/glibc/string/strsep.c.html
char *
strsep_cp (char **stringp, const char *delim)
{
  char *begin, *end;
  begin = *stringp;
  if (begin == NULL)
    return NULL;
  /* Find the end of the token.  */
  end = begin + strcspn (begin, delim);
  if (*end)
    {
      /* Terminate the token and set *STRINGP past NUL character.  */
      *end++ = '\0';
      *stringp = end;
    }
  else
    /* No more delimiters; this is the last token.  */
    *stringp = NULL;
  return begin;
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
