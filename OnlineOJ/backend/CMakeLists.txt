cmake_minimum_required(VERSION 3.16)
project(backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(OPENSSL REQUIRED openssl)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(backend ${SOURCES})

target_include_directories(backend PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${MYSQL_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    src
)

target_link_libraries(backend PRIVATE
    ${MYSQL_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)
