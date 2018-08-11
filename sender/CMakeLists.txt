cmake_minimum_required(VERSION 2.8.3)
project(sender)

# if we are on a apple machine this is needed
if (1 AND APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

set(miner_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Color.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteRGBWStrip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../pb_encode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../pb_decode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../pb_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../strip.pb.c
)

add_executable(sender ${miner_SOURCES})
target_include_directories(sender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_property(TARGET sender PROPERTY CXX_STANDARD 14)
set_property(TARGET sender PROPERTY CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2")        ## Optimize

target_link_libraries(sender
                      pthread
                     )
