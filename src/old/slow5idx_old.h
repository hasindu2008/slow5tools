/**
 * @file slow5idx_old.h
 * @brief SLOW5 random access
 * @author Hasindu Gamaarachchi (hasindu@garvan.org.au), Sasha Jenner (jenner.sasha@gmail.com)
 * @date 14/10/2020
 *
 * adapted from htslib/faidx.h
 */

#ifndef SLOW5IDX_OLD_H
#define SLOW5IDX_OLD_H

/**
 * Index SLOW5 files and extract the record for a given read ID.
 * The SLOW5 file index columns for SLOW5 are:
 *   - read ID
 *   - offset: number of bytes to skip to get to the start of the line
 *       from the beginning of the file
 *   - len: the size of the line in bytes (including \\n)
 */

#include "misc_old.h"

struct __slow5idx_t;
/// Opaque structure representing SLOW5 index
typedef struct __slow5idx_t slow5idx_t;


/** Build index for a SLOW5.
 *
 * @param fname_s5  SLOW5 file name
 * @param fname_s5i SLOW5 index file name to build
 * @param fname_gzi unused TODO remove?
 * @return          0 on success; or -1 on failure
 *
 * If fname_s5i is NULL, ".ind" will be appended to fname_s5 to make the SLOW5 index file name.
 */
int slow5idx_build3(const char *fname_s5, const char *fname_s5i, const char *fname_gzi);

/// Build index for a SLOW5.
/** @param    fname_s5 SLOW5 file name
    @return   0 on success; or -1 on failure

File [fname_s5].ind will be generated. This function is equivalent to slow5idx_build3(fname_s5, NULL, NULL).
*/
int slow5idx_build(const char *fname_s5);

/// Destroy a slow5idx_t struct
void slow5idx_destroy(slow5idx_t *slow5idx);

enum slow5idx_load_opts {
    SLOW5IDX_CREATE = 0x01,
};

/// Load SLOW5 indexes.
/** @param  fn  File name of the SLOW5 file.
    @param  fnslow5idx File name of the SLOW5 index.
    @param  fngzi unsused at the moment
    @param  flags Option flags to control index file caching and creation.
    @return Pointer to a slow5idx_t struct on success, NULL on failure.

If fnslow5idx is NULL, ".index" will be appended to fn to make the SLOW5 index file name.

If (flags & SLOW5IDX_CREATE) is true, the index files will be built using
slow5idx_build3() if they are not already present.
*/
slow5idx_t *slow5idx_load3(const char *fname_s5, const char *fname_s5i, const char *fname_gzi,
                   int flags);

/// Load index from "fn.s5i".
/** @param  fn  File name of the SLOW5 file
    @return Pointer to a slow5idx_t struct on success, NULL on failure.

This function is equivalent to slow5idx_load3(fn, NULL, NULL, SLOW5IDX_CREATE);
*/
slow5idx_t *slow5idx_load(const char *fname_s5);

//todo: reg is just readID now,
/// Fetch the sequence in a region
/** @param  slow5idx  Pointer to the slow5idx_t struct
    @param  reg  Region in the format "chr2:20,000-30,000"
    @param  len  Length of the region; -2 if seq not present, -1 general error
    @return      Pointer to the sequence; `NULL` on failure

The returned sequence is allocated by `malloc()` family and should be destroyed
by end users by calling `free()` on it.
*/
char *slow5idx_fetch(const slow5idx_t *slow5idx, const char *readid, int *len);

#endif
