cdef extern from "pyslow5.h":

    ctypedef struct slow5_file_t

# Open a slow5 file
    slow5_file_t *slow5_open(const char *pathname, const char *mode)
