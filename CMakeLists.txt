cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

project( websocket-cli )

find_package( fmt REQUIRED )
find_package( OpenSSL REQUIRED )

# --- ASIO
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/external/asio/asio/include )

# --- websocketpp
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp )

# --- CLI11
add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/external/CLI11 )

set( exec wscli )

add_executable( ${exec} )

target_sources( ${exec} PRIVATE
    src/main.cpp

    src/websocket_client.cpp
    src/websocket_client.h
)

target_link_libraries( ${exec} PRIVATE fmt::fmt CLI11::CLI11 OpenSSL::SSL OpenSSL::Crypto )

set_target_properties( ${exec} PROPERTIES CXX_STANDARD 20 )
target_compile_options( ${exec} PRIVATE -Wall -Wextra )