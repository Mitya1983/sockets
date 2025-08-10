cmake_minimum_required(VERSION 3.25)

project(sockets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_STATIC "Build static library" ON)
option(BUILD_DOCS "Build docs" OFF)
option(ENABLE_ASAN "Enables asan build" OFF)

if (${PROJECT_IS_TOP_LEVEL})
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "-g -g3 -glldb -ggdb -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wfloat-equal -Werror")
            if (${ENABLE_ASAN})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -shared-libasan")
            endif ()
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "-g0 -ggdb0 -glldb0 -O3")
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "-g -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wfloat-equal -Werror")
            if (${ENABLE_ASAN})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
            endif ()
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "-g0 -ggdb0 -O3")
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # using Visual Studio C++
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "/Zi /Od /JMC /WX")
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "/Ot /O2")
        endif ()
    else ()
        message(FATAL_ERROR "Compiler not supported")
    endif ()
endif ()

if (${PROJECT_IS_TOP_LEVEL})
    if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/openssl)
        execute_process(
                COMMAND git clone git@github.com:Mitya1983/openssl.git
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    else ()
        execute_process(
                COMMAND git pull
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl)
    endif ()
    add_subdirectory(openssl)
endif ()
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        )

file(GLOB INC_FILES CONFIGURE_DEPENDS
     include/*.hpp
     )
file(GLOB SRC_FILES CONFIGURE_DEPENDS
     source/*.cpp
     )

set(LIB_VERSION 3.1.0)

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    message(STATUS "${PROJECT_NAME}: CMAKE_INSTALL_PREFIX is not set. ${CMAKE_CURRENT_BINARY_DIR} directory will be used")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
else()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/mt_libs")
endif ()

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

set(LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/${BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Library install directory is set to ${LIBRARY_DIR}")
set(INCLUDE_DIR ${LIBRARY_DIR}/include)
message(STATUS "${PROJECT_NAME}: Include install directory is set to ${INCLUDE_DIR}")

if (NOT ${PROJECT_IS_TOP_LEVEL})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(DEBUG_BUILD)
    endif ()
    set(SOCKETS_LIB_DIR ${LIBRARY_DIR} PARENT_SCOPE)
    set(SOCKETS_INCLUDE_DIR ${INCLUDE_DIR} PARENT_SCOPE)
endif ()

if (BUILD_STATIC)
    message(STATUS "${PROJECT_NAME}: Will build static library")
    add_library(${PROJECT_NAME} STATIC)
else ()
    message(STATUS "${PROJECT_NAME}: Will build shared library")
    add_library(${PROJECT_NAME} SHARED)
endif ()

target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBS})

if (${ENABLE_ASAN} AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE "." ";" CLANG_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    list(GET CLANG_VERSION 0 CLANG_VERSION_MAJOR)
    set(PATH_TO_CLANG_DIRECTORY "/usr/lib/llvm-${CLANG_VERSION_MAJOR}/lib/clang/")

    execute_process(
            COMMAND find ${PATH_TO_CLANG_DIRECTORY} -name libclang_rt.asan-x86_64.so
            OUTPUT_VARIABLE PATH_TO_CLANG_LIB_DIRECTORY
            )
    string(STRIP ${PATH_TO_CLANG_LIB_DIRECTORY} PATH_TO_CLANG_LIB_DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE
                            ${PATH_TO_CLANG_LIB_DIRECTORY}
                            )
    target_link_libraries(${PROJECT_NAME} PRIVATE
                          -lclang_rt.asan-x86_64
                          )
endif ()

target_sources(
        ${PROJECT_NAME}
        PRIVATE ${SRC_FILES}
        PUBLIC ${INC_FILES}
        )

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        VERSION ${LIB_VERSION}
        OUTPUT_NAME ${PROJECT_NAME}
        )