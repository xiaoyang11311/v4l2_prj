cmake_minimum_required(VERSION 3.10)
project(v4l2_test)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "-g")

find_package(JPEG REQUIRED) 

#add_executable(${CMAKE_PROJECT_NAME} 
#    src/v4l2_YUYV_to_RGB.c)

add_executable(${CMAKE_PROJECT_NAME} 
    src/v4l2_MJPG_to_RGB.c)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${JPEG_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${JPEG_LIBRARY})