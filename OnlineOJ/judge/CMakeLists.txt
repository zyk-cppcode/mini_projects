cmake_minimum_required(VERSION 3.16)
project(judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(OPENSSL REQUIRED openssl)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(judge ${SOURCES})

target_include_directories(judge PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${HIREDIS_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    src
)

target_link_libraries(judge PRIVATE
    ${HIREDIS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)
