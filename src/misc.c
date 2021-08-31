/**
 * @file get.c
 * @brief get read(record) given the read_id from a SLOW5 file
 * @author Hiruna Samarakoon (h.samarakoon@garvan.org.au) Sasha Jenner (jenner.sasha@gmail.com), Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 31/08/2021
 */
#include "misc.h"

enum slow5_press_method name_to_slow5_press_method(const char *name) {
    enum slow5_press_method comp = (enum slow5_press_method) -1;

    if (strcmp(name, "none") == 0) {
        comp = SLOW5_COMPRESS_NONE;
    } else if (strcmp(name, "zlib") == 0) {
        comp = SLOW5_COMPRESS_ZLIB;
    } else if (strcmp(name, "svb-zd") == 0) {
        comp = SLOW5_COMPRESS_SVB_ZD;
    } else if (strcmp(name, "zstd") == 0) {
        comp = SLOW5_COMPRESS_ZSTD;
    }
    return comp;
}

