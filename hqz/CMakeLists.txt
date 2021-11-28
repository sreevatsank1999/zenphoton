cmake_minimum_required(VERSION 3.1)

## Adding Lib subdirectory
add_subdirectory("lib/")

project(HQZ
        VERSION 0.1.0
        LANGUAGES CXX
    )

set(HQZ_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(libHQZ_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(HQZ_INCLUDE_FILES 
        "${HQZ_INCLUDE_DIR}/hqz/zquadtree.h"
        "${HQZ_INCLUDE_DIR}/hqz/histogramimage.h"
        "${HQZ_INCLUDE_DIR}/hqz/ztrace.h"
        "${HQZ_INCLUDE_DIR}/hqz/zrender.h"
        "${HQZ_INCLUDE_DIR}/hqz/lodepng.h"
        "${HQZ_INCLUDE_DIR}/hqz/path.h"
        "${HQZ_INCLUDE_DIR}/hqz/spectrum.h"
        "${HQZ_INCLUDE_DIR}/hqz/zcheck.h"
        "${HQZ_INCLUDE_DIR}/hqz/zmaterial.h"
        "${HQZ_INCLUDE_DIR}/hqz/ray.h"
        "${HQZ_INCLUDE_DIR}/hqz/prng.h"
        "${HQZ_INCLUDE_DIR}/hqz/sampler.h"
        "${HQZ_INCLUDE_DIR}/hqz/zobject.h"
    )   

set(libHQZ_SRC_FILES
        "${libHQZ_SRC_DIR}/zrender.cpp"
        "${libHQZ_SRC_DIR}/ztrace.cpp"
        "${libHQZ_SRC_DIR}/zcheck.cpp"
        "${libHQZ_SRC_DIR}/path.cpp"
        "${libHQZ_SRC_DIR}/histogramimage.cpp"
        "${libHQZ_SRC_DIR}/spectrum.cpp"
        "${libHQZ_SRC_DIR}/lodepng.cpp"
    )

add_library(libHQZ SHARED "${HQZ_INCLUDE_FILES}" "${libHQZ_SRC_FILES}")
target_include_directories(libHQZ PUBLIC "${HQZ_INCLUDE_DIR}")
target_link_libraries(libHQZ libRapidJSON)

if(BUILD_DEBUG)
    add_compile_definitions(DEBUG=1)
endif()

set(custom_compiler_flags -Isrc -Wall -Wno-unused-value -g -O3 -march=native -ffast-math -fno-exceptions -fomit-frame-pointer -funroll-loops)

target_compile_options(libHQZ PRIVATE ${custom_compiler_flags})

##############################
## Building HQZ app binary  ##
##############################

set(binHQZ_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(binHQZ_SRC_FILES
        "${binHQZ_SRC_DIR}/main.cpp"
    )

add_executable(hqz "${HQZ_INCLUDE_FILES}" "${binHQZ_SRC_FILES}")
target_include_directories(hqz PUBLIC "${HQZ_INCLUDE_DIR}")
target_link_libraries(hqz PUBLIC libHQZ PUBLIC libRapidJSON)

target_compile_options(hqz PRIVATE ${custom_compiler_flags})