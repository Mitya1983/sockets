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

set(OPENSSL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl)
set(OPENSSL_LIB_DIR ${OPENSSL_DIR}/lib)
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/openssl)
    set(OPENSSL_EXTRACT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openssl/tmp)
    file(MAKE_DIRECTORY ${OPENSSL_EXTRACT_DIR})
    set(OPENSSL_INSTALL_DIR ${OPENSSL_EXTRACT_DIR}/install)
    file(MAKE_DIRECTORY ${OPENSSL_INSTALL_DIR})
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/openssl/openssl-3.4.0.zip DESTINATION ${OPENSSL_EXTRACT_DIR} VERBOSE)
    execute_process(
            COMMAND ./Configure --prefix=${OPENSSL_INSTALL_DIR} --openssldir=${OPENSSL_INSTALL_DIR} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
            WORKING_DIRECTORY ${OPENSSL_EXTRACT_DIR}
            )
    execute_process(
            COMMAND make
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl/tmp
            )
    execute_process(
            COMMAND make install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl/tmp
            )
    file(MAKE_DIRECTORY ${OPENSSL_DIR})
    file(COPY ${OPENSSL_INSTALL_DIR}/include DESTINATION ${OPENSSL_DIR})
    file(MAKE_DIRECTORY ${OPENSSL_LIB_DIR})
    if (BUILD_STATIC)
        file(GLOB_RECURSE OPENSSL_LIBS libssl.a libcrypto.a ${OPENSSL_INSTALL_DIR})
    else ()
        file(GLOB_RECURSE OPENSSL_LIBS libssl.so* libcrypto.so* ${OPENSSL_INSTALL_DIR})
    endif ()
    message(STATUS ${OPENSSL_LIBS})
    file(COPY ${OPENSSL_LIBS} DESTINATION ${OPENSSL_LIB_DIR})
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/openssl/tmp)
endif ()
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/openssl/include)
if (BUILD_STATIC)
    list(APPEND L_SOCKETS_DEPENDENCY_LIBS ${OPENSSL_LIB_DIR}/libssl.a ${OPENSSL_LIB_DIR}/libcrypto.a)
else ()
    list(APPEND L_SOCKETS_DEPENDENCY_LIBS ${OPENSSL_LIB_DIR}/libssl.so ${OPENSSL_LIB_DIR}/libcrypto.so)
endif ()
if (NOT ${PROJECT_IS_TOP_LEVEL})
    set(SOCKETS_DEPENDENCY_LIBS ${L_SOCKETS_DEPENDENCY_LIBS} PARENT_SCOPE)
else ()
    set(SOCKETS_DEPENDENCY_LIBS ${L_SOCKETS_DEPENDENCY_LIBS})
endif ()

include_directories(
        ${INCLUDE_DIRS}
        )

file(GLOB INC_FILES CONFIGURE_DEPENDS
     include/*.hpp
     )
file(GLOB SRC_FILES CONFIGURE_DEPENDS
     source/*.cpp
     )

set(LIB_VERSION 3.1.0)

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    message(STATUS "${PROJECT_NAME}: CMAKE_INSTALL_PREFIX is not set. ${CMAKE_BINARY_DIR} directory will be used")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/mt_libs")
else ()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/mt_libs")
endif ()

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

set(PROJECT_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME})
set(LIBRARY_INSTALL_DIR ${PROJECT_INSTALL_DIR}/${BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Library install directory is set to ${LIBRARY_INSTALL_DIR}")
set(INCLUDE_INSTALL_DIR ${LIBRARY_INSTALL_DIR}/include)
message(STATUS "${PROJECT_NAME}: Include install directory is set to ${INCLUDE_INSTALL_DIR}")

if (NOT ${PROJECT_IS_TOP_LEVEL})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(MT_DEBUG)
    endif ()
    set(SOCKETS_LIBRARY_DIR ${LIBRARY_INSTALL_DIR} PARENT_SCOPE)
    set(SOCKETS_INCLUDE_DIR ${INCLUDE_INSTALL_DIR} PARENT_SCOPE)
endif ()

if (BUILD_STATIC)
    message(STATUS "${PROJECT_NAME}: Will build static library")
    add_library(${PROJECT_NAME} STATIC)
else ()
    message(STATUS "${PROJECT_NAME}: Will build shared library")
    add_library(${PROJECT_NAME} SHARED)
endif ()

target_link_libraries(${PROJECT_NAME} ${L_SOCKETS_DEPENDENCY_LIBS})

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

