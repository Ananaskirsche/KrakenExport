cmake_minimum_required(VERSION 3.16)
project(KrakenExport)

find_package(Git REQUIRED)
find_package(OpenSSL REQUIRED)

#Check if Kraken_C_API has been downloaded already
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/libkraken/)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} clone https://github.com/hANSIc99/Kraken_C_API.git ${CMAKE_SOURCE_DIR}/external/libkraken/
            RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Error occured while running git clone")
    endif()
    EXECUTE_PROCESS(
            COMMAND sed -i "s/ =y/ =n/g" ${CMAKE_SOURCE_DIR}/external/libkraken/Makefile
    )
    EXECUTE_PROCESS(
            COMMAND make Makefile LIBRARY
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/libkraken/
    )
endif()

#Check if RapidJSON has been downloaded already
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/rapidjson/)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} clone https://github.com/Tencent/rapidjson.git ${CMAKE_SOURCE_DIR}/external/rapidjson_temp/
            RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Error occured while running git clone")
    endif()
    EXECUTE_PROCESS(
            COMMAND mv ${CMAKE_SOURCE_DIR}/external/rapidjson_temp/include/rapidjson ${CMAKE_SOURCE_DIR}/external/
    )
    EXECUTE_PROCESS(
            COMMAND rm -rf ${CMAKE_SOURCE_DIR}/external/rapidjson_temp
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(OPENSSL_USE_STATIC_LIBS TRUE)

add_executable(KrakenExport Exporter.hpp Reward.hpp CsvExporter.cpp CsvExporter.hpp main.cpp )
target_link_libraries(KrakenExport ${CMAKE_SOURCE_DIR}/external/libkraken/libkraken.a)
target_link_libraries(KrakenExport OpenSSL::Crypto)

configure_file("kraken.key" "kraken.key" COPYONLY)
configure_file("currencies.txt" "currencies.txt" COPYONLY)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")