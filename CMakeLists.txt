cmake_minimum_required(VERSION 3.17)
project(KrakenExport)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL REQUIRED)

add_executable(KrakenExport main.cpp Reward.h)
target_link_libraries(KrakenExport ${CMAKE_SOURCE_DIR}/include/libkraken/libkraken.a)
target_link_libraries(KrakenExport OpenSSL::Crypto)

configure_file("kraken.key" "kraken.key" COPYONLY)
configure_file("currencies.txt" "currencies.txt" COPYONLY)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")