cmake_minimum_required(VERSION 3.14)
project(slow5tools)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fPIC -rdynamic -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O2 -std=c++11 ")

set(CMAKE_CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/hdf5/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

set_source_files_properties(src/main.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/f2s.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/s2f.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/index.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/extract.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/slow5idx.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/thread.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/misc.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/read_fast5.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/merge_slow5.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/read_slow5.c PROPERTIES LANGUAGE CXX)

set(main src/main.c)
set(f2s src/f2s.c)
set(s2f src/s2f.c)
set(index src/index.c)
set(extract src/extract.c)
set(slow5idx src/slow5idx.c)
set(kstring src/kstring.c)
set(misc src/misc.c)
set(thread src/thread.c)
set(read_fast5 src/read_fast5.c)
set(merge_slow5 src/merge_slow5.c)
set(read_slow5 src/read_slow5.c)

set (hdf5-static "${PROJECT_SOURCE_DIR}/hdf5/lib/libhdf5.a")

add_executable(slow5tools ${main} ${f2s} ${s2f} ${index} ${extract} ${misc} ${slow5idx} ${kstring} ${thread} ${read_fast5} ${merge_slow5} ${read_slow5})

target_link_libraries(slow5tools ${hdf5-static} -lz -ldl -lm -lpthread -lrt)
