cmake_minimum_required(VERSION 3.10)

project(v4l2-encode)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "-g")

add_executable(${PROJECT_NAME} src/v4l2-encode.c)

# 包含头文件目录（include 目录）
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 链接库所在目录（lib 目录）,这个是用来查找库所在的目录的。这种一般是库不在环境变量中，库在环境变量中，就不需要这部，直接target_link_libraries即可
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib)

# 链接 x264 动态库（libx264.so），因为有上步操作，所以直接链接库名即可。
target_link_libraries(${PROJECT_NAME} PRIVATE x264)