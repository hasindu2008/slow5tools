
# distutils: language = c++
# cython: language_level=3

from libc.stdlib cimport malloc, free
from libc.string cimport strdup
from pyslow5 cimport *


def slow5_open_py(pathname, mode):
   p=str.encode(pathname)
   m=str.encode(mode)
   print(p,m)
   cdef slow5_file_t *file = slow5_open(<const char *>p, <const char *>m)
   #f = <long >file
   print(1)
