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
 -pthread \
 -lrt"
 )

# ========================================
# cmake buildsystem config

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# ========================================
# binary targets

add_library (read_cmdline  read_cmdline.c read_cmdline.h)
add_library (read_conf  read_conf.c read_conf.h)
add_library (handle_connection  handle_connection.c handle_connection.h)
add_library (handle_http  handle_http.c handle_http.h)
add_library (exec_cgi  exec_cgi.c exec_cgi.h)
add_library (async_log
  async_log.c async_log_backend.c
  async_log.h async_log_backend.h)

add_executable (muqhttpd
  muQhttpd.c read_cmdline.h read_conf.h handle_http.h async_log.h)
target_link_libraries (muqhttpd
  read_cmdline read_conf handle_connection handle_http exec_cgi async_log)
