cmake_minimum_required (VERSION 3.0)
project (muQhttpd C)

# ========================================
# compiler flags

set(CMAKE_C_FLAGS
 "-std=c99 \
 -D_XOPEN_SOURCE=600 \
 -g \
 -Wall \
 -Wextra \
 -pthread"
 )

# ========================================
# cmake buildsystem config

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# ========================================
# binary targets

add_library (read_cmdline  read_cmdline.c read_cmdline.h)
add_library (read_conf  read_conf.c read_conf.h)
add_library (handle_http  handle_http.c handle_http.h)
add_library (async_log  async_log.c async_log.h)

add_executable (muqhttpd  muQhttpd.c async_log.h read_conf.h)
target_link_libraries (muqhttpd  read_cmdline read_conf handle_http async_log)
