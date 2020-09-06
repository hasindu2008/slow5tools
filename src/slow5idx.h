/// @file slow5idx.h
/// SLOW5 random access.
/*
   adpapted from htslib/faidx.h by Hasindu Gamaarachchi <hasindu@garvan.org.au>
*/

#ifndef SLOW5IDX_H
#define SLOW5IDX_H

#include "slow5.h"

/** @file

  Index SLOW5 files and extract the record for a given read ID.

  The SLOW5 file index columns for SLOW5 are:
    - read ID
    - offset: number of bytes to skip to get to the start of the line
        from the beginning of the file
    - 0 the size of the line in bytes (including \n)

 */

struct __slow5idx_t;
/// Opaque structure representing SLOW5 index
typedef struct __slow5idx_t slow5idx_t;

/// File format to be dealing with.
enum slow5idx_format_options {
    SLOW5IDX_ASCII,
    SLOW5IDX_BINARY, //later used for binary
    SLOW5IDX_GZIP //later used for compressed
};

struct format_map {
    const char *name;
    enum slow5idx_format_options format;
};

static const struct format_map formats[] = {
    { SLOW5_NAME, SLOW5IDX_ASCII },
    { BLOW5_NAME, SLOW5IDX_BINARY},
};

/// Build index for a SLOW5.
/**  @param  fn  SLOW5 file name
     @param  fnslow5idx Name of .s5i file to build.
     @param  fngzi //unused at the moment.
     @return     0 on success; or -1 on failure

If fnslow5idx is NULL, ".s5i" will be appended to fn to make the SLOW5 index file name.
*/
int slow5idx_build3(const char *fn, const char *fnslow5idx, const char *fngzi);

/// Build index for a SLOW5.
/** @param  fn  SLOW5 file name
    @return     0 on success; or -1 on failure

File "fn.s5i" will be generated.  This function is equivalent to
*/
int slow5idx_build(const char *fn);

/// Destroy a slow5idx_t struct
void slow5idx_destroy(slow5idx_t *slow5idx);

enum slow5idx_load_options {
    SLOW5IDX_CREATE = 0x01,
};

/// Load SLOW5 indexes.
/** @param  fn  File name of the SLOW5 file.
    @param  fnslow5idx File name of the SLOW5 index.
    @param  fngzi unsused at the moment
    @param  flags Option flags to control index file caching and creation.
    @return Pointer to a slow5idx_t struct on success, NULL on failure.

If fnslow5idx is NULL, ".s5i" will be appended to fn to make the SLOW5 index file name.

If (flags & SLOW5IDX_CREATE) is true, the index files will be built using
slow5idx_build3() if they are not already present.
*/
slow5idx_t *slow5idx_load3(const char *fn, const char *fnslow5idx, const char *fngzi,
                   int flags);

/// Load index from "fn.s5i".
/** @param  fn  File name of the SLOW5 file
    @return Pointer to a slow5idx_t struct on success, NULL on failure.

This function is equivalent to slow5idx_load3(fn, NULL, NULL, SLOW5IDX_CREATE);
*/
slow5idx_t *slow5idx_load(const char *fn);

//todo: reg is just readID now,
/// Fetch the sequence in a region
/** @param  slow5idx  Pointer to the slow5idx_t struct
    @param  reg  Region in the format "chr2:20,000-30,000"
    @param  len  Length of the region; -2 if seq not present, -1 general error
    @return      Pointer to the sequence; `NULL` on failure

The returned sequence is allocated by `malloc()` family and should be destroyed
by end users by calling `free()` on it.
*/
char *slow5idx_fetch(const slow5idx_t *slow5idx, const char *reg, int *len);

#endif
