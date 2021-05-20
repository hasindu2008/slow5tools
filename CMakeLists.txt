cmake_minimum_required(VERSION 3.14)

# choose the architecture
set(DEPLOY_PLATFORM "unknown" CACHE STRING "choose the instruction set architecture")
set_property(CACHE DEPLOY_PLATFORM PROPERTY STRINGS unknown x86 arm64-v8a armeabi-v7a)
IF (${DEPLOY_PLATFORM} STREQUAL "unknown")
    message(FATAL_ERROR "choose the DEPLOY_PLATFORM")
    return() # This is to stop proceeding further and to stop opencv getting set to the default ANDROID_ABI
ENDIF()

project(slow5tools)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fPIC -rdynamic -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O2 -std=c++11 ")

set(CMAKE_CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/hdf5/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/test)

set_source_files_properties(src/main.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/f2s.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/s2f.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/index.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/view.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/klib/kstring.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/misc_old.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/thread.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/read_fast5.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/merge.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/split.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/get.c PROPERTIES LANGUAGE CXX)

set_source_files_properties(src/slow5_idx.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/slow5.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/slow5_misc.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/slow5_press.c PROPERTIES LANGUAGE CXX)

set(main src/main.c)
set(f2s src/f2s.c)
set(s2f src/s2f.c)
set(index src/index.c)
set(view src/view.c)
set(kstring src/klib/kstring.c)
set(thread src/thread.c)
set(read_fast5 src/read_fast5.c)
set(merge src/merge.c)
set(split src/split.c)
set(get src/get.c)

set(slow5idx src/slow5_idx.c)
set(slow5 src/slow5.c)
set(slow5_misc src/slow5_misc.c)
set(slow5_press src/slow5_press.c)


set (hdf5-static "${PROJECT_SOURCE_DIR}/hdf5/${DEPLOY_PLATFORM}/libhdf5.a")

add_executable(slow5tools ${main} ${f2s} ${s2f} ${index} ${view} ${kstring} ${thread} ${read_fast5} ${merge} ${split} ${get} ${slow5_misc} ${slow5} ${slow5_press} ${slow5idx})

# Build a static lib
add_library(slow5 STATIC ${slow5} ${slow5_idx} ${slow5_misc} ${slow5_press})

#just to get rid of Clion warnings
file(GLOB_RECURSE C_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "test/*.c")
add_executable(slow5test ${C_SOURCES} ${CPP_SOURCES})

# link with dependencies
IF (${DEPLOY_PLATFORM} STREQUAL "x86")
    target_link_libraries(slow5 ${hdf5-static} -lz -ldl -lm -lpthread -lrt)
    target_link_libraries(slow5tools ${hdf5-static} -lz -ldl -lm -lpthread -lrt)
ELSE()
    target_link_libraries(slow5 ${hdf5-static} -lz -ldl -lm)
    target_link_libraries(slow5tools ${hdf5-static} -lz -ldl -lm)
ENDIF()