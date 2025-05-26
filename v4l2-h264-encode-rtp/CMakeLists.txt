cmake_minimum_required(VERSION 3.10)

project(v4l2-h264-encode-rtp)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE x264)