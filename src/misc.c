/**
 * @file misc.c
 * @brief miscellaneous common functions
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

enum slow5_fmt parse_name_to_fmt(const char *fmt_str) {
    enum slow5_fmt fmt = SLOW5_FORMAT_UNKNOWN;
    for (size_t i = 0; i < sizeof PARSE_FORMAT_META / sizeof PARSE_FORMAT_META[0]; ++ i) {
        const struct parse_fmt_meta meta = PARSE_FORMAT_META[i];
        if (strcmp(meta.name, fmt_str) == 0) {
            fmt = meta.format;
            break;
        }
    }
    return fmt;
}

enum slow5_fmt parse_path_to_fmt(const char *fname) {
    enum slow5_fmt fmt = SLOW5_FORMAT_UNKNOWN;
    for (int i = strlen(fname) - 1; i >= 0; -- i) {
        if (fname[i] == '.') {
            const char *ext = fname + i;
            for (size_t j = 0; j < sizeof PARSE_FORMAT_META / sizeof PARSE_FORMAT_META[0]; ++ j) {
                const struct parse_fmt_meta meta = PARSE_FORMAT_META[j];
                if (strcmp(ext, meta.ext) == 0) { // TODO comparing the '.' is superfluous
                    fmt = meta.format;
                    break;
                }
            }
            break;
        }
    }
    return fmt;
}

int check_aux_fields_in_record(slow5_rec *slow5_record, const char *attr, int verbose){
    khint_t pos = kh_get(slow5_s2a, slow5_record->aux_map, attr);
    if(pos == kh_end(slow5_record->aux_map)){
        if(verbose){
            ERROR("Auxiliary field '%s' not found.", attr);
        }
        return -1;
    }
    return 0;
}

int check_aux_fields_in_header(slow5_hdr *slow5_header, const char *attr, int verbose){
    if(slow5_header->aux_meta->num == 0){
        ERROR("Header does not have auxiliary fields%s", "");
        return -1;
    }
    khint_t pos = kh_get(slow5_s2ui32, slow5_header->aux_meta->attr_to_pos, attr);
    if(pos == kh_end(slow5_header->aux_meta->attr_to_pos)){
        if(verbose){
            ERROR("Auxiliary field '%s' not found.", attr);
        }
        return -1;
    }
    return 0;
}

