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
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O0 -std=c++11 ")
ELSEIF()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O2 -std=c++11 ")
ENDIF()

set(CMAKE_CXX_STANDARD 11)

#to obtain prebuilt hdf5 libraries https://github.com/hiruna72/slow5tools/tree/cmake_support
include_directories(${PROJECT_SOURCE_DIR}/prebuilt-hdf5/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/slow5lib/include)
include_directories(${PROJECT_SOURCE_DIR}/slow5lib/src)
include_directories(${PROJECT_SOURCE_DIR}/test)

set_source_files_properties(src/f2s.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/get.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/index.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/merge.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/read_fast5.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/s2f.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/split.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/thread.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/view.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/stats.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/cat.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/quickcheck.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/misc.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/skim.c PROPERTIES LANGUAGE CXX)

set(f2s src/f2s.c)
set(get src/get.c)
set(index src/index.c)
set(main src/main.c)
set(merge src/merge.c)
set(read_fast5 src/read_fast5.c)
set(s2f src/s2f.c)
set(split src/split.c)
set(thread src/thread.c)
set(view src/view.c)
set(stats src/stats.c)
set(cat src/cat.c)
set(quickcheck src/quickcheck.c)
set(misc src/misc.c)
set(skim src/skim.c)

set(hdf5-static "${PROJECT_SOURCE_DIR}/prebuilt-hdf5/${DEPLOY_PLATFORM}/libhdf5.a")

add_executable(slow5tools ${f2s} ${get} ${index} ${main} ${merge} ${read_fast5} ${s2f} ${split} ${thread} ${view} ${stats} ${cat} ${quickcheck} ${misc} ${skim})

add_subdirectory(${PROJECT_SOURCE_DIR}/slow5lib)

#just to get rid of Clion warnings
file(GLOB_RECURSE C_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "test/*.c")
#add_executable(slow5test ${C_SOURCES} ${CPP_SOURCES})

# link with dependencies
IF (${DEPLOY_PLATFORM} STREQUAL "x86")
    target_link_libraries(slow5tools slow5 streamvbyte_slow5 -lhdf5_serial -lz -ldl -lm -lpthread -lrt)
#    target_link_libraries(slow5tools slow5 ${hdf5-static} -lz -ldl -lm -lpthread -lrt)
ELSE()
    target_link_libraries(slow5tools slow5 ${hdf5-static} -lz -ldl -lm)
ENDIF()